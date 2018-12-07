#ifndef _GL_MATERIAL_H
#define _GL_MATERIAL_H

#include <string>
#include <vector>

#include <veXml.h>
#include <veMath.h>

/// symbolic names for shader/material types
enum shaderType {
    SHADER_GOURAUD=0,
    SHADER_PHONG,
    SHADER_VERTEX,
    SHADER_FRAGMENT,
    SHADER_VERTEX_FRAGMENT,
};

//--- class glMatData ----------------------------------------------

/// internal class storing information about a conventional OpenGL material
class glMatData {
public:
    /// glMaterial class is allowed to access all data members
    friend class glMaterial;
protected:
    /// default constructor
    glMatData(const std::string & name="");
    /// returns xml statement containing all data
    ve::xml xml() const;
    /// returns material as VRML Appearance node
    std::string vrml(unsigned int nTabs=0) const;
    /// comparison operator equality, name is disregarded
    bool operator==(const glMatData &mat) const;

    /// stores name
    std::string m_name;
    /// stores type
    unsigned int m_type;
    /// stores color information
    ve::vec4f m_color;
    /// stores ambient color
    ve::vec3f m_ambient;
    /// stores specular color
    ve::vec3f m_specular;
    /// stores shininess
    float m_shininess;
    /// stores emissive color
    ve::vec3f m_emissive;

    /// stores texture filename
    std::string m_url;
    /// stores texture id
    unsigned int m_texId;
    /// stores texture scale
    ve::vec2f m_texScale;
    /// stores texture repetiveness in two directions
    bool m_texRepeat[2];

    /// stores reference counter
    unsigned int m_refCount;

    /// global default material in case no other is set
    static glMatData s_default;
};


//--- class glMaterial ---------------------------------------------
/// a class managing and offering public access to shared OpenGL material data
/** In order to make the application of this class most convenient, it acts as a smart pointer
to a glMatData object. */
class glMaterial {
public:
    /// default constructor
    glMaterial(const std::string & name="") {
        m_data=name.size() ? new glMatData(name) : &glMatData::s_default; }
    /// copy constructor incrementing reference count
    glMaterial(const glMaterial & source);
    /// constructor from an xml statement
    glMaterial(const ve::xml & xs);
    /// destructor
    ~glMaterial();
    /// copy operator incrementing reference count
    const glMaterial & operator=(const glMaterial & source);
        
    /// duplicates a given material, produces a physical copy
    void set(const glMaterial & source);
    /// sets data by an xml statement
    void set(const ve::xml & xs);
        
    /// returns xml statement containing all data
    ve::xml xml() const { return m_data->xml(); }
    /// returns material as VRML Appearance node
    std::string vrml(unsigned int nTabs=0) const { return m_data->vrml(); }
    /// returns type
    unsigned int type() const { return m_data->m_type; }
    /// sets type
    void type(unsigned int t) { m_data->m_type=t; }
    /// returns name
    const std::string & name() const { return m_data->m_name; }
    /// sets name
    void name(const std::string & s) { m_data->m_name=s; }
    /// comparison operator equality, name is disregarded
    bool operator==(const glMaterial &mat) const { return m_data->operator==(*mat.m_data); }

    /// sets (diffuse) RGBA color
    /** The fourth argument determines the material's alpha (i.e. 1-transparency) */
    void color ( float r, float g, float b, float a=1.0f ) {
		m_data->m_color.set(r,g,b,a); }
    /// returns (diffuse) RGBA color
    /** The fourth argument determines the material's alpha (i.e. 1-transparency) */
    const ve::vec4f & color () const { return m_data->m_color; }
    /// allows access to (diffuse) color
    ve::vec4f & color () { return m_data->m_color; }
    /// returns ambient color
    /** Since this factor is without any direct physical correspondence,  it should be  rather left untouched. */
    const ve::vec3f & ambientColor () const { return m_data->m_ambient; }
    /// allows access to ambient color
    /** Since this factor is without any direct physical correspondence,  it should be  rather left untouched. */
    ve::vec3f & ambientColor () { return m_data->m_ambient; }
    /// returns specular color
    /** Since this factor is without any direct physical correspondence,  it might be best 
    either set to 1.0 1.0 1.0 (corresponding to a plastic-like material) or to the mean of light 
    color and normal diffuse  color (corresponding to a metal-like material). */
    const ve::vec3f & specularColor () const { return m_data->m_specular; }
    /// allows access to specular color
    /** Since this factor is without any direct physical correspondence,  it might be best 
    either set to 1.0 1.0 1.0 (corresponding to a plastic-like material) or to the mean of light 
    color and normal diffuse  color (corresponding to a metal-like material). */
    ve::vec3f & specularColor () { return m_data->m_specular; }
    /// returns shininess
    /** Since this factor is without any direct physical correspondence,  it might be best 
    interpreted as specularity factor, which determines the fraction of radiance reflected direction-dependently. */
    float shininess() const { return m_data->m_shininess; }
    /// sets shininess
    /** Since this factor is without any direct physical correspondence,  it might be best 
    interpreted as specularity factor, which determines the fraction of radiance reflected direction-dependently. */
    void shininess(float f) { m_data->m_shininess=f; }
    /// returns emissive color
    /** Since this factor is without any direct physical correspondence,  it should be
     rather replaced by a uniform emissiveness factor. */
    const ve::vec3f & emissiveColor () const { return m_data->m_emissive; }
    /// allows access to emissive color
    /** Since this factor is without any direct physical correspondence,  it should be
     rather replaced by a uniform emissiveness factor. */
    ve::vec3f & emissiveColor () { return m_data->m_emissive; }
    /// returns true if material is (at least a bit) transparent. FIXME: consider also texture
    bool transparent() const { return m_data->m_color[3]<1.0f; }

    /// returns texture url
    const std::string & texName() const { return m_data->m_url; }
    /// sets name
    void texName(const std::string & s) { m_data->m_url=s; }
    /// returns texture id
    unsigned int texId() const { return m_data->m_texId; }
    /// sets texture id
    void texId(unsigned int n) { m_data->m_texId=n; }
    /// returns texture scale
    const ve::vec2f & texScale() const { return m_data->m_texScale; }
    /// allows access to texture scale
    ve::vec2f & texScale() { return m_data->m_texScale; }
    /// returns texture repetiveness
    bool texRepeat(unsigned int n=0) const { return m_data->m_texRepeat[n%2]; }
 
protected:
	/// pointer to glMatData
	glMatData * m_data;
};


//--- class materialTable ----------------------------------------
/// manages material definitions
/** Materials currently cannot be erased individually, because this may screw up the current access system based on direct array indices. */
class materialTable {
public:
    /// constructor defining a default material
    materialTable() { m_vMat.push_back(glMaterial()); m_counter=0; }
    /// clears material table
    void clear() { m_vMat.clear(); m_vMat.push_back(glMaterial()); }
    /// returns number of material entries
    unsigned int size() const { return m_vMat.size(); }

    /// reads multiple material definitions from a <Materials/> xml table
    void interpret(const ve::xml & xs);
    /// returns table as xml statement
    ve::xml xml() const;

    /// sets a material in the table
    /** In case that no material having the same name exists, a new material is added.
     \param mat the material to be set
     \return id of the added material. */
    unsigned int set(const glMaterial & mat);
    /// adds a material to the table
    /** in case that a material having the same name already exists, this method does nothing. 
     \param mat the material to be added
     \return id of the added material. */
    unsigned int add(const glMaterial & mat);
    /// adds an anonymous material to the table and generates a name
    /** in case that a material having the same properties exists, this material is reused. 
     \param mat the material to be added
     \return id of the added material. */
    unsigned int addAnonymous(const glMaterial & mat);
    /// returns material by its name or the default material if name is not found
    glMaterial & operator[](const std::string & s);
    /// returns material by its name or the default material if name is not found
    const glMaterial & operator[](const std::string & s) const;
    /// returns material by its numerical id
    const glMaterial & operator[](unsigned int n) const { return m_vMat[n%m_vMat.size()]; }
    /// returns material id by its name or the default material's id (0) if name is not found
    unsigned int getId(const std::string & s) const;
protected:    
    /// table for storing material data
    std::vector<glMaterial> m_vMat;
    /// counter for generating material names
    unsigned int m_counter;
};

#endif // _GL_MATERIAL_H
