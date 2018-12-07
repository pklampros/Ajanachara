#include <veStrUtils.h>
#include "glMaterial.h"

using namespace std;
using namespace ve;


//--- class glMatData ----------------------------------------------

glMatData glMatData::s_default("default");

glMatData::glMatData(const std::string & name) : 
    m_name(name), m_type(SHADER_PHONG), 
    m_color(0.8f,0.8f,0.8f,1.0f),
    m_ambient(0.2f,0.2f,0.2f),
    m_specular(1.0f,1.0f,1.0f),
    m_shininess(0.0f),
    m_emissive(0.0f,0.0f,0.0f),
    m_texId(0), m_texScale(1.0f,1.0f), m_refCount(1) { 
    m_texRepeat[0]=m_texRepeat[1]=true; 
}

ve::xml glMatData::xml() const {
    ve::xml xApp("Appearance");
    xApp.setAttribute("DEF",m_name);
    
    ve::xml xs("Material");
    switch(m_type) {
        case SHADER_PHONG: xs.setAttribute("type","phong"); break;
        case SHADER_VERTEX: xs.setAttribute("type","vertex_program"); break;
        case SHADER_FRAGMENT: xs.setAttribute("type","fragment_program"); break;
        case SHADER_VERTEX_FRAGMENT: xs.setAttribute("type","vertex_fragment_program"); break;
        default: break;
    }
    xs.setAttribute("diffuseColor",f2s(m_color[0])+' '+ f2s(m_color[1]) + ' ' + f2s(m_color[2]));
    if(m_color[3]<1.0f)
        xs.setAttribute("transparency",f2s(1.0f-m_color[3]));
    if((m_ambient[0]!=0.2f)||(m_ambient[1]!=0.2f)||(m_ambient[2]!=0.2f))
        xs.setAttribute("ambientColor",m_ambient.str());
    if((m_specular[0]!=1.0f)||(m_specular[1]!=1.0f)||(m_specular[2]!=1.0f))
        xs.setAttribute("specularColor",m_specular.str());
    if(m_shininess)
        xs.setAttribute("shininess",m_shininess);
    if(m_emissive[0]||m_emissive[1]||m_emissive[2])
        xs.setAttribute("emissiveColor",m_emissive.str());
    xApp.addChild(xs);

    if(m_url.size()) {
        ve::xml xTex("ImageTexture");
        xTex.setAttribute("url",m_url);
        xTex.setAttribute("texScale",m_texScale.str());
        xTex.setAttribute("repeatS",m_texRepeat[0]?"TRUE":"FALSE");
        xTex.setAttribute("repeatT",m_texRepeat[1]?"TRUE":"FALSE");
        xApp.addChild(xTex);
    }
    return xApp;
}

std::string glMatData::vrml(unsigned int nTabs) const {
    unsigned int i;
    string tabs;
    for(i=0; i<nTabs; i++) tabs+='\t';

    string s(tabs);
    if(m_name.size()) s+="DEF "+m_name+" ";

    s+=tabs+"Appearance {\n";
    s+=tabs+"\tmaterial Material {\n";
    s+=tabs+"\t\tdiffuseColor " + f2s(m_color[0])+' '+ f2s(m_color[1]) + ' ' + f2s(m_color[2]) + '\n';
    if(m_color[3]<1.0f)
        s+=tabs+"\t\ttransparency " + f2s(1.0-m_color[3]) + '\n';
    if((m_ambient[0]!=0.2f)||(m_ambient[1]!=0.2f)||(m_ambient[2]!=0.2f))
        s+=tabs+"\t\tambientColor "+m_ambient.str()+'\n';
    if((m_specular[0]!=1.0f)||(m_specular[1]!=1.0f)||(m_specular[2]!=1.0f))
        s+=tabs+"\t\tspecularColor " + f2s(m_specular[0])+' '+ f2s(m_specular[1]) + ' ' + f2s(m_specular[2]) + '\n';
    if(m_shininess)
        s+=tabs+"\t\tshininess " + f2s(m_shininess) + '\n';
    if(m_emissive[0]||m_emissive[1]||m_emissive[2])
        s+=tabs+"\t\temissiveColor " + f2s(m_emissive[0])+' '+ f2s(m_emissive[1]) + ' ' + f2s(m_emissive[2]) + '\n';
    s+=tabs+"\t}\n";
    if(m_url.size()) {
        s+=tabs+"\ttexture ImageTexture {\n";
        s+=tabs+"\t\turl [ \""+m_url+"\" ]\n";
        s+=tabs+"\t\trepeatS "+string(m_texRepeat[0]?"TRUE":"FALSE")+" repeatT "+string(m_texRepeat[1]?"TRUE":"FALSE")+"\n";
        s+=tabs+"\t}\n";
    }
    s+=tabs+"}\n";
    return s;
}

bool glMatData::operator==(const glMatData &mat) const {
    if(mat.m_type!=m_type) return false;
    if(mat.m_color!=m_color) return false;
    if(mat.m_ambient!=m_ambient) return false;
    if(mat.m_specular!=m_specular) return false;
    if(mat.m_shininess!=m_shininess) return false;
    if(mat.m_emissive!=m_emissive) return false;
    if(mat.m_url!=m_url) return false;
    if(mat.m_texId!=m_texId) return false;
    if(mat.m_texScale!=m_texScale) return false;
    if(mat.m_texRepeat[0]!=m_texRepeat[0]) return false;
    if(mat.m_texRepeat[1]!=m_texRepeat[1]) return false;
    return true;
}


//--- class glMaterial ---------------------------------------------

glMaterial::glMaterial(const ve::xml & xs) {
    m_data=&glMatData::s_default;
    set(xs);
}
    
glMaterial::glMaterial(const glMaterial & source) {
    m_data=source.m_data;
    if(m_data!=&glMatData::s_default)
        ++(m_data->m_refCount);
}

glMaterial::~glMaterial() {
    if(m_data!=&glMatData::s_default) {
        --(m_data->m_refCount);
        if(!m_data->m_refCount)
            delete m_data;
    }
}

const glMaterial & glMaterial::operator=(const glMaterial & source) {
    if(m_data!=&glMatData::s_default) {
        --(m_data->m_refCount);
        if(!m_data->m_refCount)
            delete m_data;
    }
    m_data=source.m_data;
    if(m_data!=&glMatData::s_default)
        ++(m_data->m_refCount);
    return *this;
}

void glMaterial::set(const glMaterial & source) {
    if(m_data!=&glMatData::s_default) {
        --(m_data->m_refCount);
        if(!m_data->m_refCount)
            delete m_data;
    }
    m_data=new glMatData(*source.m_data);
    m_data->m_refCount=1;
}

void glMaterial::set(const ve::xml & xs) {
    if(m_data==&glMatData::s_default)
        m_data=new glMatData(xs.getAttribute("DEF").size() ? xs.getAttribute("DEF") : xs.getAttribute("id"));
    
    const ve::xml & xTex=xs.child("ImageTexture") ? *xs.child("ImageTexture") : xs;
    const ve::xml & xMat=xs.child("Material") ? *xs.child("Material") : xs;

    m_data->m_type=SHADER_GOURAUD;
    if(xMat.getAttribute("type").size()) {
        string shader=toLower(trim(xMat.getAttribute("type")));
        if(shader=="phong") m_data->m_type=SHADER_PHONG;
        else if((shader.find("vertex")<shader.size())&&(shader.find("fragment")<shader.size()))
            m_data->m_type=SHADER_VERTEX_FRAGMENT;
        else if(shader.find("vertex")<shader.size())
            m_data->m_type=SHADER_VERTEX;
        else if(shader.find("fragment")<shader.size())
            m_data->m_type=SHADER_FRAGMENT;
    }
    if(xMat.getAttribute("diffuseColor").size())
        m_data->m_color.set(xMat.getAttribute("diffuseColor"));
    else m_data->m_color.set(0.8f,0.8f,0.8f,1.0f);
    m_data->m_color[3]=1.0f-s2f(xMat.getAttribute("transparency"));
    if(xMat.getAttribute("ambientColor").size())
        m_data->m_ambient.set(xMat.getAttribute("ambientColor"));
    else m_data->m_ambient.set(0.2f,0.2f,0.2f);
    if(xMat.getAttribute("specularColor").size())
        m_data->m_specular.set(xMat.getAttribute("specularColor"));
    else m_data->m_specular.set(1.0f,1.0f,1.0f);
    if(xMat.getAttribute("shininess").size())
        m_data->m_shininess=s2f(xMat.getAttribute("shininess"));
    else m_data->m_shininess=0.0f;
    if(xMat.getAttribute("emissiveColor").size())
        m_data->m_emissive.set(xMat.getAttribute("emissiveColor"));
    else m_data->m_emissive.set(0.0f,0.0f,0.0f);
        
    m_data->m_url=xTex.getAttribute("url");
    if(xTex.getAttribute("texScale").size())
        m_data->m_texScale.set(xTex.getAttribute("texScale"));
    else m_data->m_texScale.set(1.0f,1.0f);
    if(xTex.getAttribute("repeatS").size())
        m_data->m_texRepeat[0]=s2b(xTex.getAttribute("repeatS"));
    else m_data->m_texRepeat[0]=true;
    if(xTex.getAttribute("repeatT").size())
        m_data->m_texRepeat[1]=s2b(xTex.getAttribute("repeatT"));
    else m_data->m_texRepeat[1]=true;
}

//--- class materialTable ----------------------------------------

void materialTable::interpret(const ve::xml & xs) {
    for(unsigned int i=0; i<xs.nChildren(); ++i) 
        if((xs.child(i)->tag()=="Appearance")||(xs.child(i)->tag()=="Material"))
            m_vMat.push_back(*xs.child(i));
}

ve::xml materialTable::xml() const {
    ve::xml xs("Materials");
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        xs.addChild(m_vMat[i].xml());
    return xs;
}

unsigned int materialTable::set(const glMaterial & mat) { 
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i].name()==mat.name()) {
            m_vMat[i]=mat;
            return i;
        }
    m_vMat.push_back(mat); 
    return m_vMat.size()-1;
}
 
unsigned int materialTable::add(const glMaterial & mat) { 
    if(!mat.name().size()) return addAnonymous(mat);
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i].name()==mat.name()) return i;
    m_vMat.push_back(mat); 
    return m_vMat.size()-1;
}
 
unsigned int materialTable::addAnonymous(const glMaterial & mat) { 
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i]==mat) return i;
    m_vMat.push_back(mat); 
    m_vMat[m_vMat.size()-1].name("mat_"+i2s(m_counter++));
    return m_vMat.size()-1;
}
 
glMaterial & materialTable::operator[](const string & s) {
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i].name()==s) return m_vMat[i];
    return m_vMat[0];
}

const glMaterial & materialTable::operator[](const string & s) const {
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i].name()==s) return m_vMat[i];
    return m_vMat[0];
}

unsigned int materialTable::getId(const string & s) const {
    for(unsigned int i=0; i<m_vMat.size(); ++i)
        if(m_vMat[i].name()==s) return i;
    return 0;
}
