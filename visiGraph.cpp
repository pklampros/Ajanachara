// started 2001-11-11

#include <veStd.h>
#include <veGeoObj.h>
#include <veMath.h>
#include <veUtils.h>

#include "visiGraph.h"
#include <GL/gl.h>

#ifdef _HAVE_LIBZ
# include <zlib.h>
#endif

using namespace std;
using namespace ve;

//--- class graphNode ----------------------------------------------

graphNode::graphNode(const vector<bool> & connections ) {
    m_vEdge.reserve(connections.size());
    m_vEdge=connections;
    m_vVal.assign(2,0.0f);
	m_currVal=0;
}

unsigned int graphNode::neighbors() const {
    unsigned int n=0;
    for(unsigned int i=0; i<m_vEdge.size(); i++)
        if(m_vEdge[i]) n++;
    return n;
}

unsigned int graphNode::neighbors2(const visiGraph & graph) const {
    unsigned int i,j;
    vector<bool> tmpValue;
    tmpValue.assign(graph.size(),false);
    for(i=0; i<m_vEdge.size(); i++) if(m_vEdge[i])
        for(j=0; j<graph.size(); j++)
            if(graph[i].edge(j)&&!m_vEdge[j]) tmpValue[j]=true;
    unsigned int n=0;
    for(i=0; i<tmpValue.size(); i++) if(tmpValue[i]) n++;
    return n;
}

float graphNode::clustering(const visiGraph & graph ) const {
    float nbh=neighbors();
    if(nbh<=0.0f) return -1.0f;
    float f=0.0f;
    for(unsigned int i=0; i<m_vEdge.size(); i++) if(m_vEdge[i])
        for(unsigned int j=0; j<m_vEdge.size(); j++)
            if(m_vEdge[j]&&graph[i].edge(j)) f++;
    return f/(nbh*(nbh-1));
}

void graphNode::revelation1(float * f, const visiGraph & graph) const {
    if(m_vVal[m_currVal]<0.0f) return;
    unsigned int id=0;
    const unsigned int & szX=graph.sizeX();
    const unsigned int & szY=graph.sizeY();
    for(unsigned int k=0; k<graph.size(); k++) {
        if(m_vEdge[k]) f[k]=1;  // first order isovist
        else  f[k]=0;
        if(&graph[k]==this) id=k;
    }
    unsigned int k;
    if(id>=szX) if(m_vEdge[id-szX]) for(k=0; k<graph.size(); k++)
        if(graph[id-szX].edge(k)) if(!f[k]) f[k]=2;
    if(id<szX*szY-szX) if(m_vEdge[id+szX]) for(k=0; k<graph.size(); k++)
        if(graph[id+szX].edge(k)) if(!f[k]) f[k]=2;
    if(id%szX>0) if(m_vEdge[id-1]) for(k=0; k<graph.size(); k++)
        if(graph[id-1].edge(k)) if(!f[k]) f[k]=2;
    if(id%szX<szX-1) if(m_vEdge[id+1]) for(k=0; k<graph.size(); k++)
        if(graph[id+1].edge(k)) if(!f[k]) f[k]=2;
}

unsigned int graphNode::nRevelation( const visiGraph & graph) const {
    if(m_vVal[m_currVal]<0.0f) return 0;
    float tmpValue[graph.size()];
    revelation1(tmpValue,graph);
    unsigned int n=0;
    for(unsigned int k=0; k<graph.size(); k++)
        if(tmpValue[k]==2.0f) n++;
    return n;
}


unsigned int graphNode::perimeterLength(const visiGraph & graph,
                                        unsigned int id) const {
    if(m_vVal[m_currVal]<0.0f) return 0;
    unsigned int l=0;
    const unsigned int & szX=graph.sizeX();
    const unsigned & szY=graph.sizeY();
    for(unsigned int k=0; k<graph.size(); k++) if(m_vEdge[k]) {
        l++;
        if(k<szX) continue;
        if(!m_vEdge[k-szX]&&(k-szX!=id)) continue;
        if(k>=szX*szY-szX) continue;
        if(!m_vEdge[k+szX]&&(k+szX!=id)) continue;
        if(k%szX==0) continue;
        if(!m_vEdge[k-1]&&(k-1!=id)) continue;
        if(k%szX==szX-1) continue;
        if(!m_vEdge[k+1]&&(k+1!=id)) continue;
        l--;
    }
    return l;
}

void graphNode::perimeterOpenness(unsigned int & open, unsigned int & close,
                                  const visiGraph & graph, unsigned int id) const {
    open=close=0;
    if(m_vVal[m_currVal]<0.0f) return;
    const unsigned int & szX=graph.sizeX();
    const unsigned int & szY=graph.sizeY();

    for(unsigned int k=0; k<graph.size(); k++) if(m_vEdge[k]) {
        if(k<szX) close++;
        else if(!m_vEdge[k-szX]&&(k-szX!=id)) {
            if(graph[k].edge(k-szX)||graph[k-szX].edge(k)) open++;
            else close++;
        }
        if(k>=szX*szY-szX) close++;
        else if(!m_vEdge[k+szX]&&(k+szX!=id)) {
            if(graph[k].edge(k+szX)||graph[k+szX].edge(k)) open++;
            else close++;
        }
        if(k%szX==0) close++;
        else if(!m_vEdge[k-1]&&(k-1!=id)) {
            if(graph[k].edge(k-1)||graph[k-1].edge(k)) open++;
            else close++;
        }
        if(k%szX==szX-1) close++;
        else if(!m_vEdge[k+1]&&(k+1!=id)) {
            if(graph[k].edge(k+1)||graph[k+1].edge(k)) open++;
            else close++;
        }
    }
}

void graphNode::PCA(vec3f & vPriComp0, vec3f & vPriComp1,
                    float & minExtend0, float & maxExtend0,
                    float & minExtend1, float & maxExtend1,
                    const visiGraph & graph, unsigned int id) const {
    if(m_vVal[m_currVal]<0.0f) return;
    vector<vec3f> coord;
    vec3f standPoint(id%graph.sizeX(),id/graph.sizeX());
    //cout << endl; P(standPoint);
    coord.push_back(standPoint); // add to graph
    vec3f centroid=standPoint;
    unsigned int k;
    for(k=0; k<graph.size(); k++)
        if(m_vEdge[k]) {
            coord.push_back(vec3f(k%graph.sizeX(),k/graph.sizeX()));
            centroid.translate(k%graph.sizeX(),k/graph.sizeX());
        }
    centroid*=(1.0f/coord.size());
    //P(centroid);

    // covariance matrix:
    float a,b,d;  //c=b, symmetric matrix
    a=b=d=0;
    for(k=0; k<coord.size(); k++) {
        a+=(coord[k][X]-centroid[X])*(coord[k][X]-centroid[X]);
        b+=(coord[k][X]-centroid[X])*(coord[k][Y]-centroid[Y]);
        d+=(coord[k][Y]-centroid[Y])*(coord[k][Y]-centroid[Y]);
    }
    a*=(1.0f/coord.size());
    b*=(1.0f/coord.size());
    d*=(1.0f/coord.size());
    // eigen values:
    float eig0=0.5f*(a+d) + sqrt(b*b-a*d+0.25f*(a+d)*(a+d));
    float eig1=0.5f*(a+d) - sqrt(b*b-a*d+0.25f*(a+d)*(a+d));
    // eigen vectors:
    //PP(eig0,eig1);
#ifdef __WIN32
    if((eig0==eig1)||_isnan(eig0)||_isnan(eig1)) {
#else
    if((eig0==eig1)||isnan(eig0)||isnan(eig1)) {
#endif
        vPriComp0.set(1,0);
        vPriComp1.set(0,1);
    }
    else {
        float eigMatrix[4]= { a-eig0, b, b, d-eig0 };
        if(eigMatrix[1]!=eigMatrix[3])
            vPriComp0.set(1,(eigMatrix[2]-eigMatrix[0])/(eigMatrix[1]-eigMatrix[3]));
        else if(eigMatrix[0]!=eigMatrix[2])
            vPriComp0.set((eigMatrix[3]-eigMatrix[1])/(eigMatrix[0]-eigMatrix[2]),1);
        else vPriComp0.set(eigMatrix[1],-eigMatrix[0]);
        vPriComp0.normalize();
        vPriComp1.set(-vPriComp0[Y],vPriComp0[X]);
    }
    //PP(vPriComp0,vPriComp1);
    // now seek maximum extend in optimized coordinate system:
    minExtend0=maxExtend0=vPriComp0*(coord[0]-standPoint);
    minExtend1=maxExtend1=vPriComp1*(coord[0]-standPoint);
    for(k=1; k<coord.size(); k++) {
        float tmp=vPriComp0*(coord[k]-standPoint);
        if(tmp<minExtend0) minExtend0=tmp;
        else if(tmp>maxExtend0) maxExtend0=tmp;
        tmp=vPriComp1*(coord[k]-standPoint);
        if(tmp<minExtend1) minExtend1=tmp;
        else if(tmp>maxExtend1) maxExtend1=tmp;
    }
    //PP(minExtend0,maxExtend0);
    //PP(minExtend1,maxExtend1);
}

vector<vec3f> graphNode::isovist(const vec3f & center, const vector<triangle> & vBoundary,
                        float resolution, float tolerance, float ang0, float ang1) {
    vector<vec3f> vPolygon;
    float deltaAng= (ang1==ang0) ? 360.0f : (ang1>ang0) ? (ang1-ang0) : ang1+360.0f-ang0;
    unsigned int nRays=(unsigned int)(deltaAng/resolution);
    if(ang0!=ang1) {
        vPolygon.push_back(center);
        ++nRays;
    }
    unsigned int k;
    for(k=0; k<nRays; k++) {
        vec3f dir;
        dir.setPolar(ang0+(float)k*deltaAng/static_cast<float>(ang0!=ang1?nRays-1:nRays));
        dir.translate(center);
        line ray(center,dir);
        float minDistSqr=0;
        for(unsigned int l=0; l<vBoundary.size(); l++) {
            vec3f * pIntersect=ray.intersection(vBoundary[l],true);
            if(pIntersect) {
                float currDistSqr=center.sqrDistTo(*pIntersect);
                if((currDistSqr<minDistSqr)||!minDistSqr) minDistSqr=currDistSqr;
                delete pIntersect;
            }
        }
        dir.setPolar(ang0+(float)k*deltaAng/static_cast<float>(ang0!=ang1?nRays-1:nRays),0.0f,sqrt(minDistSqr));
        dir.translate(center);
        if((vPolygon.size()>1)&&(tolerance>0.0f)) { // optimize number of vertices:
            float deltaAngle=dAngle(vPolygon[vPolygon.size()-2].angleToXY(dir),
                                    vPolygon[vPolygon.size()-2].angleToXY(vPolygon[vPolygon.size()-1]));
            if((deltaAngle<tolerance)||(deltaAngle>360.0f-tolerance)) {
                vPolygon.pop_back();
            }
        }
        vPolygon.push_back(dir);
    }
    if(tolerance) { // optimize isovist, reduce redundant vertices:
        float deltaAngle=dAngle(vPolygon[vPolygon.size()-2].angleToXY(vPolygon[0]),
                                vPolygon[vPolygon.size()-2].angleToXY(vPolygon[vPolygon.size()-1]));
        if((deltaAngle<tolerance)||(deltaAngle>360.0f-tolerance)) {
            vPolygon.pop_back();
        }
        deltaAngle=dAngle(vPolygon[vPolygon.size()-1].angleToXY(vPolygon[1]),
                          vPolygon[vPolygon.size()-1].angleToXY(vPolygon[0]));
        if((deltaAngle<tolerance)||(deltaAngle>360.0f-tolerance))
            vPolygon.erase((vector<vec3f>::iterator)&vPolygon[0]);
    }
    return vPolygon;
}

float graphNode::visRatio(float distance, const vec3f & pos,
                          const vector<triangle> & vBoundary,
                          float resolution) const {
    if(m_vVal[m_currVal]<0.0f) return 0;

    const unsigned int nRays=(unsigned int)(360.0f/resolution);
    unsigned int nVisible=0;
    for(unsigned int k=0; k<nRays; k++) {
        vec3f dir;
        dir.setPolar((float)k*360.0f/(float)nRays,0,distance);
        dir.translate(pos);
        line ln(pos,dir);
        if(ln.intersects(vBoundary,false)) ++nVisible;
    }
    return float(nVisible)/float(nRays);
}


string graphNode::labels(const string & sep) {
    return "X"+sep
        +"Y"+sep
        +"neighbors"+sep
#ifdef _ALL_MEASURANDS
        +"neighbors2"+sep
#endif
        +"clustering"+sep
        +"revelation"+sep
        +"perimeter"+sep
        +"open"+sep
        +"close"+sep
#ifdef _ALL_MEASURANDS
        +"pc0angle"+sep
        +"pc0minExt"+sep
        +"pc0maxExt"+sep
        +"pc1minExt"+sep
        +"pc1maxExt"+sep
        +"boundingProp"+sep
#endif
        +"nVertices"+sep
        +"visRatio2"+sep
        +"visRatio4"+sep
        +"visRatio6"+sep
        +"visRatio8";
}

string graphNode::measurands(const visiGraph & graph,
                             unsigned int id, const vec3f & pos, const string & sep) const {
    unsigned int open, close;
    perimeterOpenness(open,close,graph,id);
    vec3f pc0, pc1;
    float minExt0, maxExt0, minExt1, maxExt1;
    PCA(pc0,pc1, minExt0,maxExt0, minExt1,maxExt1, graph,id);
    vector<vec3f> vPolygon=isovist(pos,graph.boundary(),2,.2);

    return i2s(id%graph.sizeX())+sep
        +i2s(id/graph.sizeX())+sep
        +i2s(neighbors())+sep
#ifdef _ALL_MEASURANDS
        +i2s(neighbors2(graph))+sep
#endif
        +f2s(clustering(graph))+sep
        +i2s(nRevelation(graph))+sep
        +i2s(perimeterLength(graph,id))+sep
        +i2s(open)+sep
        +i2s(close)+sep
#ifdef _ALL_MEASURANDS
        +f2s(datan(pc0[Y]/pc0[X]))+sep
        +f2s(minExt0)+sep
        +f2s(maxExt0)+sep
        +f2s(minExt1)+sep
        +f2s(maxExt1)+sep
        +f2s((maxExt0-minExt0)/(maxExt1-minExt1))+sep
#endif
        +i2s(vPolygon.size())+sep
        +f2s(visRatio(2.0f,pos,graph.boundary(),2))+sep
        +f2s(visRatio(4.0f,pos,graph.boundary(),2))+sep
        +f2s(visRatio(6.0f,pos,graph.boundary(),2))+sep
        +f2s(visRatio(8.0f,pos,graph.boundary(),2));
}

//--- class visiGraph ----------------------------------------------

visiGraph::visiGraph() : isDisplayed(true), m_bColor(true) {
    periResolution=5;
    periAngleTolerance=5;
    m_ang0=m_ang1=0.0f;
    clear();
}

visiGraph::visiGraph(ve::xmlIni & ini) : isDisplayed(true), m_bColor(true) {
    ini.focusOn("visibilityGraphAnalysis");
    ini.read(orig[Z],"heightOverGround",1.6);
    ini.read(m_res,"graphResolution",0.5);
    ini.read(orig[X],"originX",0.0);
    ini.read(orig[Y],"originY",0.0);
    ini.read(m_ignoreTransp,"ignoreTranspWalls",true);
    ini.read(periResolution,"periResolution",5.0f);
    ini.read(periAngleTolerance,"periAngleTolerance",5.0f);
    ini.focusOff();

    m_ang0=m_ang1=0.0f;
    clear();
}

visiGraph::~visiGraph() {
    clear();
}

void visiGraph::clear() {
    vVertex.clear();
    m_vNode.clear();
    vBoundary.clear();
    vPermeability.clear();
    szX=0;
    szY=0;
    m_state=VGR_STATE_NONE;
    boundaryModelName="";
    m_graphFileName="";
    m_progress=-1.0f;
    m_step=0;
    m_ignoreTransp=true;
}

void visiGraph::draw() {
    unsigned int i,j;
    switch(m_state) {
    case VGR_STATE_NONE:
    case VGR_STATE_COMPUTE:
    case VGR_STATE_CLUSTERING:
    case VGR_STATE_NBH2:
    case VGR_STATE_NVTX:
        return;
    case VGR_STATE_POSITION:
        szX=(unsigned int)((maxCoord[X]-minCoord[X])/m_res+1);
        szY=(unsigned int)((maxCoord[Y]-minCoord[Y])/m_res+1);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glColor4f(1.0,0.5,0.5,0.5);
        glBegin(GL_QUADS);
        for(j=0; j<szY; j++) for(i=0; i<szX; i++) {
            glVertex3f(orig[X] + minCoord[X]+i*m_res -m_res*0.45f,orig[Y] + minCoord[Y]+j*m_res -m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X] + minCoord[X]+i*m_res +m_res*0.45f,orig[Y] + minCoord[Y]+j*m_res -m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X] + minCoord[X]+i*m_res +m_res*0.45f,orig[Y] + minCoord[Y]+j*m_res +m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X] + minCoord[X]+i*m_res -m_res*0.45f,orig[Y] + minCoord[Y]+j*m_res +m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
        }
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        return;
    case VGR_STATE_ADDREMOVE:
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBegin(GL_QUADS);
        for(j=0; j<szY; j++) for(i=0; i<szX; i++) {
            if(m_vNode[i+szX*j].value()>0.0f) glColor4f(0,1,0,0.5);
            else glColor4f(1,0,0,0.5);
            glVertex3f(orig[X]+i*m_res -m_res*0.45f,orig[Y]+j*m_res -m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X]+i*m_res +m_res*0.45f,orig[Y]+j*m_res -m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X]+i*m_res +m_res*0.45f,orig[Y]+j*m_res +m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
            glVertex3f(orig[X]+i*m_res -m_res*0.45f,orig[Y]+j*m_res +m_res*0.45f,(minCoord[Z]+maxCoord[Z])*0.5f);
        }
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        return;
    default:
        break;
    }
    if(isDisplayed) { // display normal visibility graph
        glBegin(GL_QUADS);
        for(j=0; j<szY; j++) for(i=0; i<szX; i++) if(m_vNode[i+szX*j].value()>=0.0f) {
            float value=float(m_vNode[i+szX*j].value()-m_valueMin)/float(m_valueMax-m_valueMin);
            
            if(m_bColor)
                glColor3f(std::min(2.0f*value,1.0f),std::min(2.0f*(value-0.5f),1.0f),std::max(0.0f,0.5f-value));
            else glColor3f(value,value,value);
            glVertex3f(orig[X]+i*m_res -m_res*0.5f,orig[Y]+j*m_res -m_res*0.5f,orig[Z]);
            glVertex3f(orig[X]+i*m_res +m_res*0.5f,orig[Y]+j*m_res -m_res*0.5f,orig[Z]);
            glVertex3f(orig[X]+i*m_res +m_res*0.5f,orig[Y]+j*m_res +m_res*0.5f,orig[Z]);
            glVertex3f(orig[X]+i*m_res -m_res*0.5f,orig[Y]+j*m_res +m_res*0.5f,orig[Z]);
        }
        glEnd();
    }
    switch(m_state) { // special display:
    case VGR_STATE_PCA:
        glBegin(GL_LINES);
        glColor3f(1,0,0);
        glVertex3f(orig[X]+standPoint[X]*m_res,
                   orig[Y]+standPoint[Y]*m_res,orig[Z]);
        glVertex3f(orig[X]+(standPoint[X]+vEig[0][X]*maxExtend0)*m_res,
                   orig[Y]+(standPoint[Y]+vEig[0][Y]*maxExtend0)*m_res,orig[Z]);
        glVertex3f(orig[X]+standPoint[X]*m_res,
                   orig[Y]+standPoint[Y]*m_res,orig[Z]);
        glVertex3f(orig[X]+(standPoint[X]+vEig[0][X]*minExtend0)*m_res,
                   orig[Y]+(standPoint[Y]+vEig[0][Y]*minExtend0)*m_res,orig[Z]);
        glColor3f(0,1,0);
        glVertex3f(orig[X]+standPoint[X]*m_res,
                   orig[Y]+standPoint[Y]*m_res,orig[Z]);
        glVertex3f(orig[X]+(standPoint[X]+vEig[1][X]*maxExtend1)*m_res,
                   orig[Y]+(standPoint[Y]+vEig[1][Y]*maxExtend1)*m_res,orig[Z]);
        glVertex3f(orig[X]+standPoint[X]*m_res,
                   orig[Y]+standPoint[Y]*m_res,orig[Z]);
        glVertex3f(orig[X]+(standPoint[X]+vEig[1][X]*minExtend1)*m_res,
                   orig[Y]+(standPoint[Y]+vEig[1][Y]*minExtend1)*m_res,orig[Z]);
        glEnd();
        break;
    case VGR_STATE_PROFILE:
    case VGR_STATE_PARTIAL:
        if(!vPolyBoundary.size()) break;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.0f,1.0f,0.0f,0.4f);
        glVertex3fv(&m_lastIntersection[X]);
        for(i=0; i<vPolyBoundary.size(); i++)
            glVertex3f(vPolyBoundary[i][X],vPolyBoundary[i][Y],orig[Z]);
        glVertex3f(vPolyBoundary[0][X],vPolyBoundary[0][Y],orig[Z]);
        glEnd();
        glBegin(GL_QUADS);
        glColor4f(1.0f,1.0f,0.5f,0.7f);
        for(i=0; i<vPolyBoundary.size(); i++) {
            glVertex3f(vPolyBoundary[i][X] -m_res*0.2f,vPolyBoundary[i][Y] -m_res*0.2f,orig[Z]);
            glVertex3f(vPolyBoundary[i][X] +m_res*0.2f,vPolyBoundary[i][Y] -m_res*0.2f,orig[Z]);
            glVertex3f(vPolyBoundary[i][X] +m_res*0.2f,vPolyBoundary[i][Y] +m_res*0.2f,orig[Z]);
            glVertex3f(vPolyBoundary[i][X] -m_res*0.2f,vPolyBoundary[i][Y] +m_res*0.2f,orig[Z]);
        }
        glColor4f(1.0f,1.0f,0.0f,1.0f);
        glVertex3f(m_lastIntersection[X] -m_res*0.3f,m_lastIntersection[Y] -m_res*0.3f,orig[Z]);
        glVertex3f(m_lastIntersection[X] +m_res*0.3f,m_lastIntersection[Y] -m_res*0.3f,orig[Z]);
        glVertex3f(m_lastIntersection[X] +m_res*0.3f,m_lastIntersection[Y] +m_res*0.3f,orig[Z]);
        glVertex3f(m_lastIntersection[X] -m_res*0.3f,m_lastIntersection[Y] +m_res*0.3f,orig[Z]);
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        break;
    default:
        break;
    }
}

bool visiGraph::intersects(const ve::line & ray, unsigned int & i, unsigned int & j) {
    for(j=0; j<szY; j++) for(i=0; i<szX; i++) {
        triangle tr1(orig[X]+i*m_res -m_res*0.5f,orig[Y]+j*m_res -m_res*0.5f,orig[Z],
                     orig[X]+i*m_res +m_res*0.5f,orig[Y]+j*m_res -m_res*0.5f,orig[Z],
                     orig[X]+i*m_res +m_res*0.5f,orig[Y]+j*m_res +m_res*0.5f,orig[Z]);
        triangle tr2(orig[X]+i*m_res -m_res*0.5f,orig[Y]+j*m_res -m_res*0.5f,orig[Z],
                     orig[X]+i*m_res +m_res*0.5f,orig[Y]+j*m_res +m_res*0.5f,orig[Z],
                     orig[X]+i*m_res -m_res*0.5f,orig[Y]+j*m_res +m_res*0.5f,orig[Z]);
        vec3f * pTest=ray.intersection(tr1);
        if(!pTest) pTest=ray.intersection(tr2);
        if(pTest) {
            m_lastIntersection=*pTest;
            delete pTest;
            return true;
        }
    }
    return false;
}

void visiGraph::nextStep() {
    unsigned int i,j;
    switch(m_state) {
    case VGR_STATE_POSITION:
        orig[X]+=minCoord[X];
        orig[Y]+=minCoord[Y];
        vVertex.reserve(szX*szY);
        for(j=0; j<szY; j++) for(i=0; i<szX; i++) {
            m_vNode.push_back(graphNode(szX*szY));
            line ln(orig[X]+i*m_res,orig[Y]+j*m_res,maxCoord[Z],
                    orig[X]+i*m_res,orig[Y]+j*m_res,minCoord[Z]);
            if(ln.intersects(vPermeability,true))
                m_vNode[m_vNode.size()-1].value()=1.0f;
            else m_vNode[m_vNode.size()-1].value()=-1.0f;
            vVertex.push_back(vec3f(orig[X]+i*m_res,orig[Y]+j*m_res,orig[Z]));
        }
        m_msg="Please edit vertices and afterwards press OK.";
        m_state++;
        break;
    case VGR_STATE_ADDREMOVE:
        m_state++;
        m_msg="computing graph...";
        m_progress=0.0f;
        break;
    case VGR_STATE_COMPUTE: {
        if(m_step<m_vNode.size()-1) {
            if(m_vNode[m_step].value()>0.0f) for(j=m_step+1; j<m_vNode.size(); j++) if(m_vNode[j].value()>0.0f)
                if(!line(vVertex[m_step],vVertex[j]).intersects(vBoundary,false)) {
                    m_vNode[m_step].edge()[j]=true;
                    m_vNode[j].edge()[m_step]=true;
                }
            ++m_step;
            m_progress=float(m_step+1)/float(vVertex.size());
        }
        else {
            computeNeighborhood();
            m_msg="computing graph done.";
            m_state=VGR_STATE_COMPUTATION_DONE;
        }
        break;
    }
    case VGR_STATE_COMPUTATION_DONE:
        m_progress=-1.0f;
        m_step=0;
        m_state=VGR_STATE_READY;
        break;
    case VGR_STATE_CLUSTERING:
        if(m_step<m_vNode.size()) {
            if(m_vNode[m_step].value()>=0.0f)
                m_vNode[m_step].value()=m_vNode[m_step].clustering(*this);
            m_step++;
            m_progress=float(m_step+1)/float(m_vNode.size());
        }
        else {
            updateRange();
            m_msg="Clustering coefficient, min="+f2s(m_valueMin)+", max="+f2s(m_valueMax);
            m_state=VGR_STATE_COMPUTATION_DONE;
        }
        break;
    case VGR_STATE_NBH2: {
        for(unsigned int i=0; i<10; i++) if(m_step<m_vNode.size()) {
            if(m_vNode[m_step].value()>=0.0f)
                m_vNode[m_step].value()=m_vNode[m_step].neighbors2(*this);
            m_step++;
            m_progress=float(m_step+1)/float(m_vNode.size());
        }
        else {
            updateRange();
            m_msg="Second order neighborhood size, min="+i2s((int)m_valueMin)+", max="+i2s((int)m_valueMax);
            m_state=VGR_STATE_COMPUTATION_DONE;
        }
        break;
    }
    case VGR_STATE_NVTX: {
        for(unsigned int i=0; i<10; i++) if(m_step<m_vNode.size()) {
            if(m_vNode[m_step].value()>=0.0f) {
                vec3f pos(orig[X]+(m_step%szX)*m_res,orig[Y]+(m_step/szX)*m_res,orig[Z]);
                m_vNode[m_step].value()=graphNode::isovist(pos,boundary(),2,.2,m_ang0,m_ang1).size();
            }
            m_step++;
            m_progress=float(m_step+1)/float(m_vNode.size());
        }
        else {
            updateRange();
            m_msg="Isovist polygon n vertices, min="+i2s((int)m_valueMin)+", max="+i2s((int)m_valueMax);
            m_state=VGR_STATE_COMPUTATION_DONE;
        }
        break;
    }
    default:
        break;
    }
}

void visiGraph::switchCell(const line & ray) {
    if(!VGR_STATE_ADDREMOVE) return;
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    m_vNode[i+szX*j].value()=(m_vNode[i+szX*j].value()>0.0f) ? -1 : 1;
    m_msg="vertex ("+i2s(i)+','+i2s(j)+") flipped.";
}

void visiGraph::generate(const ve::geoNode & permeability) {
    clear();
    permeability.triangles(vPermeability);
	vector<vec3f> vVertex;
	permeability.vertices(vVertex);
	if(vVertex.size()) {
		minCoord=maxCoord=vVertex[0];
		for(unsigned int i=0; i<vVertex.size(); ++i) {
			if(vVertex[i][X]<minCoord[X]) minCoord[X]=vVertex[i][X];
			else if(vVertex[i][X]>maxCoord[X]) maxCoord[X]=vVertex[i][X];
			if(vVertex[i][Y]<minCoord[Y]) minCoord[Y]=vVertex[i][Y];
			else if(vVertex[i][Y]>maxCoord[Y]) maxCoord[Y]=vVertex[i][Y];
			if(vVertex[i][Z]<minCoord[Z]) minCoord[Z]=vVertex[i][Z];
			else if(vVertex[i][Z]>maxCoord[Z]) maxCoord[Z]=vVertex[i][Z];
		}
	}
    minCoord[Z]-=1.0f;
    maxCoord[Z]+=1.0f;
    szX=(unsigned int)((maxCoord[X]-minCoord[X])/m_res+1);
    szY=(unsigned int)((maxCoord[Y]-minCoord[Y])/m_res+1);
    m_state=VGR_STATE_POSITION;
}


void visiGraph::boundaryModel(const ve::geoNode & boundary, const string & name) {
    if(name.size()) boundaryModelName=fileIo::unifyPath(name);
    if(boundaryModelName.find('/')<boundaryModelName.size())
        boundaryModelName=boundaryModelName.substr(boundaryModelName.rfind('/')+1);
    vBoundary.clear();
# if 0 // currently disabled due to change to hierarchical model
    if(m_ignoreTransp) {
        // currently non-recursive, descends only 1 step!
        for(unsigned int j=0; j<boundary.size(); j++) {
            // interpret all transparency as completely transparent
            if(!(boundary.node(j)->flags()&&GEO_FLAG_TRANSPARENT))
                boundary.node(j)->triangles(vBoundary);
        }
    } else
#endif
    boundary.triangles(vBoundary);
}

string visiGraph::str() const {
    if(m_state<VGR_STATE_READY) return "visibility graph not ready.\n";
    string s;
    for(unsigned int i=0; i<m_vNode.size(); i++) {
        for(unsigned int j=0; j<m_vNode[i].size(); j++) {
            if(m_vNode[i].edge(j)) s+="1 ";
            else s+="0 ";
        }
        s[s.size()-1]='\n';
    }
    return s;
}

int visiGraph::save(const string & fileName) {
    if(m_state<VGR_STATE_READY) return 1;
    m_graphFileName=fileName;
#ifdef _HAVE_LIBZ
    string hdr="% Visibility graph generated by "+cmdLine::name()+" v"+cmdLine::version()
        +" (c) "+cmdLine::date()+" by "+cmdLine::author()+'\n';
    hdr+="% model: "+boundaryModelName+" originX: "+f2s(orig[X])
        +" originY: "+f2s(orig[Y])+" resolution: "+f2s(m_res)+" height: "+f2s(orig[Z])+" sizeX: "+i2s(szX)+" sizeY: "+i2s(szY)+'\n';
    gzFile fp=gzopen(fileName.c_str(),"wb");
    if (!fp) return 1;
    gzwrite(fp,(void*)hdr.c_str(),hdr.size());
    gzwrite(fp,(void*)str().c_str(),str().size());
    gzclose(fp);
#else
    return 1;
#endif
    return 0;
}

int visiGraph::load(const string & fileName) {
    bool fileIsValid=false;
#ifdef _HAVE_LIBZ
    gzFile fp=gzopen(fileName.c_str(),"rb");
    if (!fp) {
        m_msg="Loading graph \""+fileName+"\" failed.";
        return 1;
    }
    m_vNode.clear();
    vBoundary.clear();

    unsigned int bufSz=4096; // initial size. should hopefully be enough for all headers
    char * buffer= new char[bufSz];
    while(!gzeof(fp)) { // main reading loop
        if(!fileIsValid) gzgets(fp,buffer,bufSz);
        else gzread(fp,buffer,bufSz);
        if(buffer[0]) {
            if(!fileIsValid&&((buffer[0]=='%')||(buffer[0]=='#'))) {
                vector<string> word;
                split(&buffer[1],word);
                if((word[0]=="model:")&&(word.size()>9)) {
                    boundaryModelName=word[1];
                    orig[X]=s2f(word[3]);
                    orig[Y]=s2f(word[5]);
                    m_res    =s2f(word[7]);
                    orig[Z] =s2f(word[9]);
                    szX=s2i(word[11]);
                    szY=s2i(word[13]);
                    bufSz=szX*szY*2;
                    delete [] buffer;
                    buffer= new char[bufSz];
                    fileIsValid=true;
                }
            }
            else if(fileIsValid) {
                vector<bool> vBool;
                vBool.assign(szX*szY,false);
                for(unsigned int i=0; i<szX*szY; i++)
                    if(buffer[i*2]=='1') vBool[i]=true;
                m_vNode.push_back(vBool);
            }
        }
    }
    delete [] buffer;
    gzclose(fp);
#endif
    if (!fileIsValid) {
        m_msg="Interpreting graph \""+fileName+"\" failed.";
        m_state=VGR_STATE_NONE;
        return 1;
    }

    m_graphFileName=fileName;
    m_msg="Visibility graph \""+m_graphFileName+"\" loaded.";
    m_state=VGR_STATE_COMPUTATION_DONE;
    return 0;
}

int visiGraph::saveAnalysis(const string & fileName) const {
    if(m_state<VGR_STATE_READY) return 1;
    ofstream file(fileName.c_str(), std::ios::out );
    if (!file.good()) return 1;
    for(unsigned int i=0; i<szY; i++) {
        for(unsigned int j=szX; j>0; j--)
            file << m_vNode[i*szX+j-1].value() << ' ';
        file << '\n';
    }
    file.close();
    return 0;
}

void visiGraph::updateRange() {
    bool firstTime=true;
    for(unsigned int i=0; i<m_vNode.size(); i++) if(m_vNode[i].value()>=0.0f) {
        if(firstTime) {
            m_valueMin=m_valueMax=m_vNode[i].value();
            firstTime=false;
        }
        else if(m_vNode[i].value()<m_valueMin) m_valueMin=m_vNode[i].value();
        else if(m_vNode[i].value()>m_valueMax) m_valueMax=m_vNode[i].value();
    }
}

void visiGraph::computeNeighborhood() {
    for(unsigned int i=0; i<m_vNode.size(); i++) if(m_vNode[i].value()>=0.0f) {
        m_vNode[i].value()=m_vNode[i].neighbors();
        if(m_vNode[i].value()<=0.0f) m_vNode[i].value()=-1.0f;
    }
    updateRange();
    m_msg="Neighborhood size, min="+i2s((int)m_valueMin)+", max="+i2s((int)m_valueMax);
    m_state=VGR_STATE_READY;
}

float visiGraph::value(const ve::line & ray) {
    if(m_state<VGR_STATE_READY) return -1;
    unsigned int i,j;
    if(!intersects(ray,i,j)) return -1;
    return m_vNode[i+szX*j].value();
}

float visiGraph::value(float x, float y) {
    ve::line ray(vec3f(x,y,maxCoord[Z]+10.0f),vec3f(x,y,minCoord[Z]-10.0f));
    return value(ray);
}

void visiGraph::showIsovist(const ve::line & ray, float ang0, float ang1) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(ang0==ang1) { // complete isovist
        for(unsigned int k=0; k<m_vNode.size(); k++)
            if(m_vNode[k].value()>=0.0f) m_vNode[k].value()=m_vNode[i+szX*j].edge(k);
        m_msg="Isovist vertex ("+i2s(i)+','+i2s(j)+")";
    }
    else { // partial isovist
        for(unsigned int k=0; k<m_vNode.size(); k++) if(m_vNode[k].value()>=0.0f) {
            m_vNode[k].value()=0.0f;
            float ang=vec2f(i,j).angleTo(vec2f(k%szX,k/szX));
            if(ang1>ang0) {
                if(m_vNode[i+szX*j].edge(k)&&(ang>=ang0)&&(ang<=ang1))
                    m_vNode[k].value()=1.0f;
            }
            else {
                if(m_vNode[i+szX*j].edge(k)&&((ang>=ang0)||(ang<=ang1)))
                    m_vNode[k].value()=1.0f;
            }
        }
        m_msg="Isovist vertex ("+i2s(i)+','+i2s(j)+") from angle "+f2s(ang0,2)+" to "+f2s(ang1,2);
    }
    m_vNode[i+szX*j].value()=1.5;
    m_valueMin=0.0f;
    m_valueMax=1.5f;
}

void visiGraph::showIsovist2(const ve::line & ray) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    unsigned int k;
    for(k=0; k<m_vNode.size(); k++)
        if(m_vNode[k].value()>=0.0f) m_vNode[k].value()= m_vNode[i+szX*j].edge(k) ? 0.5f : 0.0f;
    for(k=0; k<m_vNode.size(); k++)
        if(m_vNode[k].value()==0.5f) for(unsigned int l=0; l<m_vNode.size(); l++)
            if((m_vNode[l].value()==0.0f)&&m_vNode[k].edge(l))
                m_vNode[l].value()= 1.0f;
    m_vNode[i+szX*j].value()=1.5;
    m_valueMin=0.0f;
    m_valueMax=1.5f;
    m_msg="Second order isovist vertex ("+i2s(i)+','+i2s(j)+")";
}

void visiGraph::showRevelation1(const ve::line & ray) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(m_vNode[i+szX*j].value()<0.0f) return;
    float tmpValue[m_vNode.size()];
    m_vNode[i+szX*j].revelation1(tmpValue,*this);
    for(unsigned int k=0; k<m_vNode.size(); k++)
        if(m_vNode[k].value()>=0.0f) m_vNode[k].value()=tmpValue[k];
    m_valueMin=0.0f;
    m_valueMax=2.0f;
    m_msg="Revelation 1 step vertex ("+i2s(i)+','+i2s(j)+")";
}

void visiGraph::computeRevelation1() {
    for(unsigned int j=0; j<szY; j++) for(unsigned int i=0; i<szX; i++)
        if(m_vNode[i+szX*j].value()>=0.0f)
            m_vNode[i+szX*j].value()=m_vNode[i+szX*j].nRevelation(*this);
    updateRange();
    m_msg="Revelation 1 step, min="+f2s(m_valueMin)+", max="+f2s(m_valueMax);
    m_state=VGR_STATE_READY;
}

void visiGraph::showClustering(const ve::line & ray) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(m_vNode[i+szX*j].value()<0.0f) return;
    m_vNode[i+szX*j].clustering(*this);
    for(unsigned int k=0; k<m_vNode[i+szX*j].size(); k++) {
        if(m_vNode[k].value()>0.0f) m_vNode[k].value()=0.0f;
        if(m_vNode[i+szX*j].edge(k))
            for(unsigned int l=0; l<m_vNode[i+szX*j].size(); l++)
                if(m_vNode[i+szX*j].edge(l)&&m_vNode[k].edge(l)) m_vNode[k].value()++;
    }

    m_valueMin=0.0f;
    m_valueMax=m_vNode[i+szX*j].neighbors();
    m_msg="Clustering vertex ("+i2s(i)+','+i2s(j)+")";
}

void visiGraph::showPerimeter(const ve::line & ray, float ang0, float ang1) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(m_vNode[i+szX*j].value()<0.0f) return;
    unsigned int id=i+szX*j;
    // calculate on graph:
    unsigned int k;
    for(k=0; k<m_vNode.size(); k++) if(m_vNode[id].edge(k)) {
        m_vNode[k].value()=1.0f;
        if(k<szX) continue;
        if(!m_vNode[id].edge(k-szX)&&(k-szX!=id)) continue;
        if(k>=szX*szY-szX) continue;
        if(!m_vNode[id].edge(k+szX)&&(k+szX!=id)) continue;
        if(k%szX==0) continue;
        if(!m_vNode[id].edge(k-1)&&(k-1!=id)) continue;
        if(k%szX==szX-1) continue;
        if(!m_vNode[id].edge(k+1)&&(k+1!=id)) continue;
        m_vNode[k].value()=0.0f;
    }
    else if(m_vNode[k].value()>0.0f) m_vNode[k].value()=0.0f;
    for(k=0; k<m_vNode.size(); k++) if(m_vNode[k].value()) { // test whether it is an open or closed edge
        bool isClosed=true;
        if((k>=szX)&&(k-szX!=id))
            if(m_vNode[k].edge(k-szX)||m_vNode[k-szX].edge(k)) 
                if(!m_vNode[id].edge(k-szX)) isClosed=false;
        if(isClosed&&(k<szX*szY-szX)&&(k+szX!=id))
            if(m_vNode[k].edge(k+szX)||m_vNode[k+szX].edge(k))
                if(!m_vNode[id].edge(k+szX)) isClosed=false;                
        if(isClosed&&(k%szX!=0)&&(k-1!=id)) 
            if(m_vNode[k].edge(k-1)||m_vNode[k-1].edge(k))
                if(!m_vNode[id].edge(k-1)) isClosed=false;
        if(isClosed&&(k%szX!=szX-1)&&(k+1!=id)) 
            if(m_vNode[k].edge(k+1)||m_vNode[k+1].edge(k))
                if(!m_vNode[id].edge(k+1)) isClosed=false;
        if(!isClosed) m_vNode[k].value()=0.5;
    }
    m_vNode[id].value()=1.5f;

    m_valueMin=0.0f;
    m_valueMax=1.5f;

    // create bounding polygon:
    vec3f center(orig[X]+i*m_res,orig[Y]+j*m_res,orig[Z]);
    vPolyBoundary=graphNode::isovist(center,vBoundary,periResolution,periAngleTolerance,ang0,ang1);
    m_msg="Isovist perimeter vertex ("+i2s(i)+','+i2s(j)+"), nVertices="+i2s(vPolyBoundary.size());
}

void visiGraph::computePerimeterLength() {
    for(unsigned int i=0; i<m_vNode.size(); i++) {
        if(m_vNode[i].value()<0.0f) continue;
        m_vNode[i].value()=m_vNode[i].perimeterLength(*this,i);
    }
    updateRange();
    m_msg="Isovist perimeter length, min="+i2s((int)m_valueMin)+", max="+i2s((int)m_valueMax);
    m_state=VGR_STATE_READY;
}

void visiGraph::computePerimeterDivArea() {
    for(unsigned int i=0; i<szX*szY; i++) {
        if(m_vNode[i].value()<0.0f) continue;
        m_vNode[i].value()=m_vNode[i].perimeterLength(*this,i);
        m_vNode[i].value()*=m_vNode[i].value()/m_vNode[i].neighbors();
    }
    updateRange();
    m_msg="Isovist perimeter² / area, min="+f2s(m_valueMin)+", max="+f2s(m_valueMax);
    m_state=VGR_STATE_READY;
}

void visiGraph::computePerimeterOpenness() {
    for(unsigned int i=0; i<szX*szY; i++) {
        if(m_vNode[i].value()<0.0f) continue;

        unsigned int open, close;
        m_vNode[i].perimeterOpenness(open,close,*this,i);
        m_vNode[i].value()=(float)open/((float)close+(float)open);
    }
    updateRange();
    m_msg="Isovist perimeter openness ratio, min="+f2s(m_valueMin)+", max="+f2s(m_valueMax);
    m_state=VGR_STATE_READY;
}

string visiGraph::measurands(const ve::line & ray) {
    unsigned int i,j;
    if(m_state<VGR_STATE_READY) return "";
    if(!intersects(ray,i,j)) return "";
    if(m_vNode[i+szX*j].value()<0.0f) return "";
    unsigned int id=i+szX*j;
    vec3f center(orig[X]+i*m_res,orig[Y]+j*m_res,orig[Z]);

    return graphNode::labels()+'\n'+m_vNode[id].measurands(*this,id,center)+'\n';
}

string visiGraph::profile(const ve::line & ray, float resolution, float ang0, float ang1) {
    unsigned int i,j;
    if(m_state<VGR_STATE_READY) return "";
    if(!intersects(ray,i,j)) return "";
    if(m_vNode[i+szX*j].value()<0.0f) return "";

    // create bounding polygon:
    vec3f center(orig[X]+i*m_res,orig[Y]+j*m_res,orig[Z]);
    vPolyBoundary=graphNode::isovist(center,vBoundary,resolution,0.0f, ang0, ang1);
    m_msg="Isovist perimeter vertex ("+i2s(i)+','+i2s(j)+"), nVertices="+i2s(vPolyBoundary.size());

    string sep(" ");
    string s(i2s(i)+sep+i2s(j));
    for(unsigned int k=0; k<vPolyBoundary.size(); k++)
        s+=sep+f2s(center.distTo(vPolyBoundary[k]));
    s+='\n';
    return s;
}

string visiGraph::partial(const ve::line & ray, float resolution, float ang0, float ang1) {
    unsigned int i,j;
    if(m_state<VGR_STATE_READY) return "";
    vPolyBoundary.clear();
    if(!intersects(ray,i,j)) return "";
    if(m_vNode[i+szX*j].value()<0.0f) return "";

    // create bounding polygon and calculate basic data:
    vPolyBoundary=graphNode::isovist(m_lastIntersection,vBoundary,periResolution,periAngleTolerance,ang0,ang1);
    float peri=0.0f, area=0.0f;
    float open=0.0f, close=0.0f;
    const float openRes=0.25f; // resolution for openness calculation
    for(i=1; i<=vPolyBoundary.size(); ++i) {
        float length=vPolyBoundary[i-1].distTo(vPolyBoundary[i%vPolyBoundary.size()]);
        peri+=length;
        area+=triangle(m_lastIntersection,vPolyBoundary[i-1],vPolyBoundary[i%vPolyBoundary.size()]).area();
        
        unsigned int nSegs=static_cast<unsigned int>(length/openRes);
        if(!nSegs) continue;
        vec3f dir(vPolyBoundary[i-1],vPolyBoundary[i%vPolyBoundary.size()]);
        dir.normalize();
        dir*=openRes;
        vec3f nor(-dir[Y],dir[X]);
        vec3f pos(vPolyBoundary[i-1]);
        pos+=dir*0.5f;
        for(j=0;j<nSegs; ++j) {
            line ray(pos+nor,pos-nor);
            if(ray.intersects(vBoundary,false)) ++close; 
            else ++open;
            pos+=dir;
        }
    }
    float openness=(open+close)? open/(open+close) : 0.0f;
    float jagged=peri*peri/area;
    
    m_msg="pos=("+f2s(m_lastIntersection[X])+";"+f2s(m_lastIntersection[Y])
        +"), angle=("+f2s(ang0)+";"+f2s(ang1)+"), nVertices="
        +i2s(vPolyBoundary.size())+ ", area="+f2s(area)+", perimeter="
        +f2s(peri)+", openness="+f2s(openness)+", jaggedness="+f2s(jagged);
    return "x y ang0 ang1 nvertices area perimeter openness jaggedness\n"
        +f2s(m_lastIntersection[X])+" "+f2s(m_lastIntersection[Y])
        +" "+f2s(ang0)+" "+f2s(ang1)+" "+i2s(vPolyBoundary.size())
        +" "+f2s(area)+" "+f2s(peri)+" "+f2s(openness)+" "+f2s(jagged)+"\n";
}

void visiGraph::showPCA(const ve::line & ray) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(m_vNode[i+szX*j].value()<0.0f) return;
    standPoint.set(i,j);

    m_vNode[i+szX*j].PCA(vEig[0],vEig[1], minExtend0, maxExtend0,
                       minExtend1,maxExtend1, *this,i+szX*j);

    for(unsigned int k=0; k<m_vNode.size(); k++)
        if(m_vNode[k].value()>=0.0f) m_vNode[k].value()=m_vNode[i+szX*j].edge(k);
    m_vNode[i+szX*j].value()=1.5;
    m_valueMin=0.0f;
    m_valueMax=1.5f;
    m_msg="Isovist principal components vertex ("+i2s(i)+','+i2s(j)+")";
}

void visiGraph::showCircum(const ve::line & ray, float radius) {
    unsigned int i,j;
    if(!intersects(ray,i,j)) return;
    if(m_vNode[i+szX*j].value()<0.0f) return;

    standPoint.set(i,j);
    for(unsigned int k=0; k<m_vNode.size(); k++)
        if(m_vNode[k].value()>=0.0f) {
            vec3f testPoint(k%szX,k/szX);
            float dist=standPoint.distTo(testPoint);
            if(fabs(dist*m_res-radius)<m_res*0.5f) {
                if(m_vNode[i+szX*j].edge(k)) m_vNode[k].value()=1;
                else m_vNode[k].value()=0.5f;
            }
            else m_vNode[k].value()=0;
        }
    m_vNode[i+szX*j].value()=1.5;
    m_valueMin=0.0f;
    m_valueMax=1.5f;

    vec3f pos(orig[X]+i*m_res,orig[Y]+j*m_res,orig[Z]);
    float visibility=m_vNode[i+szX*j].visRatio(radius,pos, vBoundary,periResolution);
    m_msg="Visibility ratio at distance "+f2s(radius)+"m, vertex ("+i2s(i)+','+i2s(j)+") = "+f2s(visibility);
}

void visiGraph::calcVisRatio(float radius) {
    // first calculate maximum number of possible vertices:
    float nVertices=0.0f;
    unsigned int szLine=(unsigned int)(radius*2.0f/m_res)+3;
    unsigned char * matrix=new unsigned char[szLine*szLine];
    memset(matrix,0,szLine*szLine);
    vec3f center(radius/m_res+1,radius/m_res+1);
    for(unsigned int k=0; k<szLine*szLine; ++k) {
        vec3f testPoint(k%szLine,k/szLine);
        float dist=center.distTo(testPoint);
        if(fabs(dist*m_res-radius)<m_res*0.5f) ++nVertices;
    }
    delete[] matrix;
    // now calculate on actual graph:
    for(unsigned int j=0; j<szY; ++j) for(unsigned int i=0; i<szX; ++i)
        if(m_vNode[i+szX*j].value()>=0.0f) {
            center.set(i,j,0);
            float nVerticesConnected=0;
            for(unsigned int k=0; k<m_vNode.size(); k++)
                if(m_vNode[k].value()>=0.0f) {
                    vec3f testPoint(k%szX,k/szX);
                    float dist=center.distTo(testPoint);
                    if(fabs(dist*m_res-radius)<m_res*0.5f) {
                        if(m_vNode[i+szX*j].edge(k)) ++nVerticesConnected;
                    }
                }
            m_vNode[i+szX*j].value()=nVerticesConnected/nVertices;
        }
    // finish visualization:
    updateRange();
    m_msg="Visibility ratio at distance "+f2s(radius)+", min="
        +f2s(m_valueMin)+", max="+f2s(m_valueMax);
    m_state=VGR_STATE_READY;
}

void visiGraph::calcMinDist() {
    unsigned int k;
    bool isBorder;
    for(unsigned int i=0; i<m_vNode.size(); ++i) {
        if(m_vNode[i].value()<0.0f) continue;
		// test if node is border itself:
		isBorder=false;
		if(i<szX) isBorder=true;
		else if(!m_vNode[i].edge(i-szX)) isBorder=true;
		else if(i>=szX*szY-szX) isBorder=true;
		else if(!m_vNode[i].edge(i+szX)) isBorder=true;
		else if(i%szX==0) isBorder=true;
		else if(!m_vNode[i].edge(i-1)) isBorder=true;
		else if(i%szX==szX-1) isBorder=true;
		else if(!m_vNode[i].edge(i+1)) isBorder=true;
		if(isBorder) {
			m_vNode[i].value()=0.5f*m_res;
		}
		else {
			// test connected nodes:
			float minDistSqr=FLT_MAX;
			vec3f center(i%szX,i/szX);
			for(k=0; k<m_vNode.size(); k++) if(m_vNode[i].edge(k)) {
				isBorder=false;
				if(k<szX) isBorder=true;
				else if(!m_vNode[i].edge(k-szX)&&(k-szX!=i)) isBorder=true;
				else if(k>=szX*szY-szX) isBorder=true;
				else if(!m_vNode[i].edge(k+szX)&&(k+szX!=i)) isBorder=true;
				else if(k%szX==0) isBorder=true;
				else if(!m_vNode[i].edge(k-1)&&(k-1!=i)) isBorder=true;
				else if(k%szX==szX-1) isBorder=true;
				else if(!m_vNode[i].edge(k+1)&&(k+1!=i)) isBorder=true;
				if(isBorder) {
					vec3f testPoint(k%szX,k/szX);
					float currDistSqr=center.sqrDistTo(testPoint);
					minDistSqr=min(minDistSqr,currDistSqr);
				}
			}
			m_vNode[i].value()=sqrt(minDistSqr)*m_res;
		}
    }
    // finish visualization:
    updateRange();
    m_msg="minimum wall distance, min="+f2s(m_valueMin)+", max="+f2s(m_valueMax);
    m_state=VGR_STATE_READY;
}

void visiGraph::calcExtrema(bool testMinima) {
	unsigned int valueIndex= m_vNode[0].current() ? 0 : 1;
    unsigned int i;
	// first detect extrema and saddle points:
    for(i=0; i<m_vNode.size(); ++i) {
		if(m_vNode[i].value()<0.0f) {
			m_vNode[i].value(valueIndex)=-1;
			continue;
		}
		m_vNode[i].value(valueIndex)=0;
		if(testMinima) {
			if(i>=szX) { // test upper row
				if(i%szX) if(m_vNode[i-szX-1].value()>=0.0f && m_vNode[i-szX-1].value()<m_vNode[i].value())
					continue;
				if(m_vNode[i-szX].value()>=0.0f && m_vNode[i-szX].value()<m_vNode[i].value())
					continue;
				if(i%szX!=szX-1) if(m_vNode[i-szX+1].value()>=0.0f && m_vNode[i-szX+1].value()<m_vNode[i].value())
					continue;
			}			
			if(i%szX) if(m_vNode[i-1].value()>=0.0f && m_vNode[i-1].value()<m_vNode[i].value())
				continue;
			if(i%szX!=szX-1) if(m_vNode[i+1].value()>=0.0f && m_vNode[i+1].value()<m_vNode[i].value())
				continue;
			if(i<szX*szY-szX) { // test lower row
				if(i%szX) if(m_vNode[i+szX-1].value()>=0.0f && m_vNode[i+szX-1].value()<m_vNode[i].value())
					continue;
				if(m_vNode[i+szX].value()>=0.0f && m_vNode[i+szX].value()<m_vNode[i].value())
					continue;
				if(i%szX!=szX-1) if(m_vNode[i+szX+1].value()>=0.0f && m_vNode[i+szX+1].value()<m_vNode[i].value())
					continue;
			}	
		}
		else {
			if(i>=szX) { // test upper row
				if(i%szX) if(m_vNode[i-szX-1].value()>m_vNode[i].value())
					continue;
				if(m_vNode[i-szX].value()>m_vNode[i].value())
					continue;
				if(i%szX!=szX-1) if(m_vNode[i-szX+1].value()>m_vNode[i].value())
					continue;
			}			
			if(i%szX) if(m_vNode[i-1].value()>m_vNode[i].value())
				continue;
			if(i%szX!=szX-1) if(m_vNode[i+1].value()>m_vNode[i].value())
				continue;
			if(i<szX*szY-szX) { // test lower row
				if(i%szX) if(m_vNode[i+szX-1].value()>m_vNode[i].value())
					continue;
				if(m_vNode[i+szX].value()>m_vNode[i].value())
					continue;
				if(i%szX!=szX-1) if(m_vNode[i+szX+1].value()>m_vNode[i].value())
					continue;
			}	
		}
		m_vNode[i].value(valueIndex)=1.0f;
    }
	// now remove saddle points, i.e. extrema that have neighbors having the same value that are no extremum:
    for(i=0; i<m_vNode.size(); ++i) {
		if(m_vNode[i].value(valueIndex)==1.0f) {
			bool saddle=false;
			if(i>=szX) { // test upper row
				if(i%szX) if((m_vNode[i-szX-1].value()==m_vNode[i].value())&&(m_vNode[i-szX-1].value(valueIndex)!=1.0f))
					saddle=true;
				if((m_vNode[i-szX].value()==m_vNode[i].value())&&(m_vNode[i-szX].value(valueIndex)!=1.0f))
					saddle=true;
				if(i%szX!=szX-1) if((m_vNode[i-szX+1].value()==m_vNode[i].value())&&(m_vNode[i-szX+1].value(valueIndex)!=1.0f))
					saddle=true;
			}			
			if(i%szX) if((m_vNode[i-1].value()==m_vNode[i].value())&&(m_vNode[i-1].value(valueIndex)!=1.0f))
				saddle=true;
			if(i%szX!=szX-1) if((m_vNode[i+1].value()==m_vNode[i].value())&&(m_vNode[i+1].value(valueIndex)!=1.0f))
				saddle=true;
			if((i<szX*szY-szX)&&!saddle) { // test lower row
				if(i%szX) if((m_vNode[i+szX-1].value()==m_vNode[i].value())&&(m_vNode[i+szX-1].value(valueIndex)!=1.0f))
					saddle=true;
				if((m_vNode[i+szX].value()==m_vNode[i].value())&&(m_vNode[i+szX].value(valueIndex)!=1.0f))
					saddle=true;
				if(i%szX!=szX-1) if((m_vNode[i+szX+1].value()==m_vNode[i].value())&&(m_vNode[i+szX+1].value(valueIndex)!=1.0f))
					saddle=true;
			}	
			if(saddle) {
				m_vNode[i].value(valueIndex)=0.25f;
				i=0; // redo from start
			}
		}
	}
	// then detect connected extrema:
	unsigned int currRegion=0;
	vector<unsigned int> vRegion;
	vRegion.assign(m_vNode.size(),0);
    for(i=0; i<m_vNode.size(); ++i) {
		if(m_vNode[i].value(valueIndex)==1.0f) {
			if(i>=szX) { // test upper row
				if(i%szX) if(m_vNode[i-szX-1].value(valueIndex)==1.0f)
					vRegion[i]=vRegion[i-szX-1];
				if(m_vNode[i-szX].value(valueIndex)==1.0f) {
					if(vRegion[i]&&(vRegion[i-szX]!=vRegion[i])) {
						for(unsigned int j=0; j<i; ++j)
							if(vRegion[j]==vRegion[i-szX]) vRegion[j]=vRegion[i];
					}
					else vRegion[i]=vRegion[i-szX];
				}
				if(i%szX!=szX-1) if(m_vNode[i-szX+1].value(valueIndex)==1.0f) {
					if(vRegion[i]&&(vRegion[i-szX+1]!=vRegion[i])) {
						for(unsigned int j=0; j<i; ++j)
							if(vRegion[j]==vRegion[i-szX+1]) vRegion[j]=vRegion[i];
					}
					else vRegion[i]=vRegion[i-szX+1];
				}
			}			
			if(i%szX) if(m_vNode[i-1].value(valueIndex)==1.0f) {
				if(vRegion[i]&&(vRegion[i-1]!=vRegion[i])) {
					for(unsigned int j=0; j<i; ++j)
						if(vRegion[j]==vRegion[i-1]) vRegion[j]=vRegion[i];
				}
				else vRegion[i]=vRegion[i-1];
			}	
			if(!vRegion[i]) {
				vRegion[i]=++currRegion;
			}
		}
	}
	// finally merge connected extrema:
	unsigned int nExtrema=0;
	for(unsigned int j=1;j<currRegion;++j) {
		vec2f center(0.0f,0.0f);
		unsigned int nCells=0;
		for(i=0; i<vRegion.size(); ++i) if(vRegion[i]==j) {
			++nCells;
			m_vNode[i].value(valueIndex)=0.5f;
			center.translate(i%szX,i/szX);
		}
		if(nCells) {
			center/=nCells;
			unsigned int index=static_cast<unsigned int>(center[X])+szX*static_cast<unsigned int>(center[Y]);
			m_vNode[index].value(valueIndex)=1.0f; // might be outside of extreme region if things go bad!!
			++nExtrema;
		}
	}
	
	// make new calculation visible:
    for(i=0; i<m_vNode.size(); ++i) 
		m_vNode[i].current((m_vNode[i].current()+1)%m_vNode[i].nValues());
    m_valueMin=0.0f;
    m_valueMax=1.0f;
	if(testMinima)
		m_msg="n minima: "+i2s(nExtrema);
	else m_msg="n maxima: "+i2s(nExtrema);
}
