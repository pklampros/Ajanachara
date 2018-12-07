#include "veIoObj.h"
#include "glMaterial.h"
#include <veGeoObj.h>
#include <veStrUtils.h>
#include <veUtils.h>

using namespace ve;
using namespace std;


//--- loading ------------------------------------------------------

static int loadMtl(const std::string & filename, vector<glMaterial> & vMat ) {
    ifstream file(filename.c_str(), std::ios::in);
    if (!file.good()) {
        cerr << "ioObj::loadMtl() ERROR: " << filename << " file error or file not found!\n";
        return 1;
    }
    
    glMaterial* pMtl=0;
    string line;
    while(!file.eof()) {
        getline(file,line);
        line=trim(line);
        vector<string> vWord;
        split(line,vWord);
        
        if(vWord.size()<2) continue;
        if(vWord[0]=="newmtl") {
            if(pMtl) {
                vMat.push_back(*pMtl);
                delete pMtl;
            }
            pMtl= new glMaterial(vWord[1]);
            continue;
        }
        if(!pMtl) continue;
        if(vWord[0]=="Ns")
            pMtl->shininess(s2f(vWord[1]));
        else if((vWord[0]=="Ka")&&(vWord.size()>3))
            pMtl->ambientColor().set(s2f(vWord[1]),s2f(vWord[2]),s2f(vWord[3]));
        else if((vWord[0]=="Kd")&&(vWord.size()>3))
            pMtl->color(s2f(vWord[1]),s2f(vWord[2]),s2f(vWord[3]),pMtl->color()[3]);
        else if((vWord[0]=="Ks")&&(vWord.size()>3))
            pMtl->specularColor().set(s2f(vWord[1]),s2f(vWord[2]),s2f(vWord[3]));
        else if((vWord[0]=="d")||(vWord[0]=="Tr"))
            pMtl->color()[3]=s2f(vWord[1]);
        else if(vWord[0]=="illum") {
            if(vWord[1]=="2") pMtl->type(SHADER_PHONG);
        }
        else if(vWord[0].find("map")<vWord[0].size()) 
            pMtl->texName(vWord[1]);
    }
    file.close();
    if(pMtl) vMat.push_back(*pMtl);
    return 0;
}

geoNode * ioObj::load(const std::string & filename) {
    string fname=fileIo::unifyPath(filename);
    unsigned int filenamePos=fname.rfind('/');
    if(filenamePos<fname.size())
        fileIo::addSearchPath(fname.substr(0,filenamePos+1));
    ifstream file(fname.c_str(), std::ios::in);
    if (!file.good()) {
        cerr << "ve::ioObj::load() ERROR: " << fname << " file error or file not found!\n";
        return 0;
    }
    
    vector<glMaterial> vMat;
    geoGroup * pParent=new geoGroup;
    pParent->name(filename);
    
    string line;
    geoMesh * pMesh=new geoMesh;
    bool faceMode=false;
    unsigned int vIndexOffset=1;
    unsigned int tIndexOffset=1;
    unsigned int nIndexOffset=1;
    
    while(!file.eof()) {
        getline(file,line);
        line=trim(line);
        vector<string> vWord;
        split(line,vWord);
        
        if(vWord.size()&&(vWord[0]=="f")) {
            faceMode=true;
            if(vWord.size()>3) {
                for(unsigned int i=1; i<vWord.size(); ++i) {
                    unsigned int slashPos=vWord[i].find('/');
                    int index=s2i(vWord[i].substr(0,slashPos));
                    if(index<0) index+=pMesh->indices().size();
                    else index-=vIndexOffset;
                    pMesh->indices().push_back(index);
                    //cout << "v:" << index << " ";
                    if(slashPos<vWord[i].size()) {
                        index=s2i(vWord[i].substr(slashPos+1,vWord[i].rfind('/')+1-slashPos));
                        if(index<0) pMesh->texIndices().push_back(pMesh->texIndices().size()+index);
                        else if(index>0) pMesh->texIndices().push_back(index-tIndexOffset);
                        //if(index) cout << "t:" << index-tIndexOffset << " ";
                        
                        if(vWord[i].find('/',slashPos+1)<vWord[i].size()) {
                            index=s2i(vWord[i].substr(vWord[i].rfind('/')+1,vWord[i].size()));
                            if(index<0) pMesh->normalIndices().push_back(pMesh->normalIndices().size()+index);
                            else if(index>0) pMesh->normalIndices().push_back(index-nIndexOffset);
                            //if(index) cout << "n:" << index-nIndexOffset;
                        }
                    }
                    //cout << endl;
                }
                pMesh->faceEnds().push_back(pMesh->indices().size()-1);
            }
            continue;
        }
        else if(faceMode) { // mesh is over
            pParent->addNode(pMesh,false);
            vIndexOffset+=pMesh->coords().size();
            tIndexOffset+=pMesh->texCoords().size();
            nIndexOffset+=pMesh->normals().size();
            pMesh=new geoMesh;
            faceMode=false;
        }
        if(!line.size()||(line[0]=='#')) continue;
            
        if(((vWord[0]=="g")||(vWord[0]=="o"))&&(vWord.size()>1))
            pMesh->name(vWord[1]);
        else if((vWord[0]=="v")&&(vWord.size()>3))
            pMesh->coords().push_back(vec3f(s2f(vWord[1]),s2f(vWord[2]),s2f(vWord[3])));
        else if((vWord[0]=="vt")&&(vWord.size()>2))
            pMesh->texCoords().push_back(vec2f(s2f(vWord[1]),s2f(vWord[2])));
        else if((vWord[0]=="vn")&&(vWord.size()>3))
            pMesh->normals().push_back(vec3f(s2f(vWord[1]),s2f(vWord[2]),s2f(vWord[3])));
        else if((vWord[0]=="usemtl")&&(vWord.size()>1)) {
            for(unsigned int i=0; i<vMat.size(); ++i)
                if(vMat[i].name()==vWord[1]) {
                    pMesh->matName(vMat[i].name());
                    pMesh->color()=vMat[i].color();
                    pMesh->specularColor()=vMat[i].specularColor();
                    pMesh->textureFileName(vMat[i].texName());
                    pMesh->textureRepeat()=vMat[i].texRepeat();
                    break;
                }
        }
        else if(vWord[0]=="mtllib") for(unsigned int i=1; i<vWord.size();++i) {
            if(filenamePos<fname.size())
                loadMtl(fname.substr(0,filenamePos+1)+vWord[i],vMat);
            else loadMtl(vWord[i],vMat);
        }
    }
    file.close();
    
    delete pMesh;
    if(pParent->size()==1) {
        pMesh=new geoMesh(*static_cast<geoMesh*>(pParent->node(0)));
        delete pParent;
        return pMesh;
    }
    else if(pParent->size()) return pParent;
    delete pParent;
    return 0;
}


//--- saving -------------------------------------------------------

/// remove transformations from the scene:
static void flattenTransforms(geoNode & node) {
    if(node.type()==GEO_TRANSFORM) {
        geoTransform & transf=*((geoTransform*)&node);
        for(unsigned int i=0; i<node.size(); ++i)
            node.node(i)->transform(transf.transform());
        mat4f identity;
        transf.setTransform(identity);
    }
    for(unsigned int i=0; i<node.size(); ++i)
        flattenTransforms(*node.node(i));
}

/// recursively parses scene graph and converts meshes into obj
static void mesh2obj(geoNode & node, string & obj, materialTable & matTable, 
    unsigned int & vIndexOffset, unsigned int & tIndexOffset, unsigned int & nIndexOffset) {
    if(node.type()==GEO_MESH) {
        const geoMesh & mesh=*static_cast<const geoMesh*>(&node);
        // add material data:
        if(mesh.name().size())
            obj+="o "+mesh.name()+'\n';
        else obj+="o mesh"+i2s(mesh.id())+'\n';
        glMaterial mat(mesh.matName());
        mat.color()=mesh.color();
        mat.specularColor()=mesh.specularColor();
        mat.texName(mesh.textureFileName());
        unsigned int id=matTable.add(mat);
        obj+="usemtl "+matTable[id].name()+'\n';
        // add vertex data:
        for(unsigned int i=0; i<mesh.coords().size(); ++i)
            obj+="v "+mesh.coords()[i].str()+'\n';
        for(unsigned int i=0; i<mesh.texCoords().size(); ++i)
            obj+="vt "+mesh.texCoords()[i].str()+'\n';
        for(unsigned int i=0; i<mesh.normals().size(); ++i)
            obj+="vn "+mesh.normals()[i].str()+'\n';
        // add indices:
        bool tIndex=(mesh.texIndices().size()==mesh.indices().size());
        bool nIndex=(mesh.normalIndices().size()==mesh.indices().size());
        if(mesh.faceEnds().size()&&mesh.indices().size()) {
            obj+="f";
            unsigned int currFaceEnd=0;
            for(unsigned int i=0;i<mesh.indices().size(); ++i) {
                obj+=" "+i2s(mesh.indices()[i]+vIndexOffset);
                if(tIndex||nIndex) obj+='/';
                if(tIndex) obj+=i2s(mesh.texIndices()[i]+tIndexOffset);
                if(nIndex) obj+='/'+i2s(mesh.normalIndices()[i]+nIndexOffset);
                if(i==mesh.faceEnds()[currFaceEnd]) {
                    ++currFaceEnd;
                    if(currFaceEnd<mesh.faceEnds().size())
                        obj+="\nf";
                    else obj+="\n\n";
                }
            }
        }
        vIndexOffset+=mesh.coords().size();
        tIndexOffset+=mesh.texCoords().size();
        nIndexOffset+=mesh.normals().size();
    }
    else for(unsigned int i=0; i<node.size(); ++i)
        mesh2obj(*node.node(i),obj,matTable,vIndexOffset,tIndexOffset,nIndexOffset);
}


int ioObj::save(const ve::geoNode & model, const std::string & filename) {
    // first flatten all transforms:
    geoNodePtr pModel=const_cast<geoNode&>(model).copy(true);
    flattenTransforms(*pModel);

    // collect geometry and material:
    string mtlFileName=filename.substr(0,filename.rfind('.')+1)+"mtl";
    string obj("# "+filename+"\nmtllib "+mtlFileName+"\n\n");
    materialTable matTable;
    unsigned int vIndexOffset=1;
    unsigned int tIndexOffset=1;
    unsigned int nIndexOffset=1;
    mesh2obj(*pModel,obj,matTable,vIndexOffset,tIndexOffset,nIndexOffset);
    
    // save OBJ file:
    ofstream file(filename.c_str(), std::ios::out);
    if (!file.good()) {
        cerr << "ve::ioObj::save() ERROR: " << filename << " file error.\n";
        return 1;
    }
    file << obj << flush;
    file.close();
    
    // generate and save MTL file:    
    file.open(mtlFileName.c_str(), std::ios::out);
    if (!file.good()) {
        cerr << "ve::ioObj::save() ERROR: " << mtlFileName << " file error.\n";
        return 1;
    }
    for(unsigned int i=0; i<matTable.size(); ++i) {
        file<< "newmtl " << matTable[i].name() << '\n'
            << "Ns " << matTable[i].shininess() << '\n'
            << "Ka " << matTable[i].ambientColor()[0] << ' ' << matTable[i].ambientColor()[1]
                << ' ' << matTable[i].ambientColor()[2] << '\n'
            << "Kd " << matTable[i].color()[0] << ' ' << matTable[i].color()[1] 
                << ' ' << matTable[i].color()[2] << '\n'
            << "Ks " << matTable[i].specularColor()[0] << ' ' << matTable[i].specularColor()[1] 
                << ' ' << matTable[i].specularColor()[2] << '\n'
            << "d " << matTable[i].color()[3] << '\n'
            << "illum " << (matTable[i].type()==SHADER_PHONG ? 2 : 1) << '\n';
        if(matTable[i].texName().size())
             file << "map_Kd " << matTable[i].texName() << "\n\n" << flush;
        else file << endl;
    }
    
    file.close();    
    return 0;
}
