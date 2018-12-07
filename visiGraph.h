#ifndef _VISIGRAPH_H
#define _VISIGRAPH_H

#include <veStd.h>
#include <veGeoObj.h>
#include <veMath.h>

//--- class graphNode ----------------------------------------------- /*FOLD00*/
class visiGraph;
/// a class for representing graph vertices
class graphNode {
public:
    /// default constructor
    graphNode(unsigned int nEdgesMax=0 )
    { m_vEdge.assign(nEdgesMax,false); m_vVal.assign(2,0.0f); m_currVal=0; };
    /// constructor taking a vector of bool connections
    graphNode(const std::vector<bool> & connections );
    
	/// returns a reference of the current characteristic value
    float & value() { return m_vVal[m_currVal]; };
    /// returns the current characteristic value
    float value() const { return m_vVal[m_currVal]; };
    /// returns a reference of characteristic value n
    float & value(unsigned int n) { return m_vVal[n]; };
    /// returns characteristic value n
    float value(unsigned int n) const { return m_vVal[n]; };
	/// returns id of current value
	unsigned int current() const { return m_currVal; };
	/// sets id of current value
	void current( unsigned int n ) { if(n<m_vVal.size()) m_currVal=n; }
	/// returns number of values
	unsigned int nValues() const { return m_vVal.size(); }
	
    /// allows access to connections to other vertices
    std::vector<bool> & edge() { return m_vEdge; };
    /// returns connection i to other vertices
    bool edge(unsigned int i) const { return m_vEdge[i]; };
    /// returns number of edges
    unsigned int size() const { return m_vEdge.size(); };

    /// returns all measurand labels
    static std::string labels(const std::string & sep=" ");
    /// computes and returns all measurands
    std::string measurands(const visiGraph & graph, unsigned int id, const ve::vec3f & pos,
                           const std::string & sep=" ") const;
    /// returns neighborhood size
    unsigned int neighbors() const;
    /// returns second order neighborhood size
    unsigned int neighbors2(const visiGraph & graph ) const;
    /// returns clustering coefficient
    float clustering(const visiGraph & graph ) const;
    /// computes revealed vertices by 1 step and stores it in f
    void revelation1(float * f, const visiGraph & graph) const;
    /// returns number of revealed vertices by 1 step
    unsigned int nRevelation(const visiGraph & graph) const;
    /// returns perimeter length
    unsigned int perimeterLength( const visiGraph & graph, unsigned int id) const;
    /// stores number of open and closed isovist boundaries in open and close
    void perimeterOpenness( unsigned int & open, unsigned int & close,
                           const visiGraph & graph, unsigned int id) const;
    /// computes principal components and extend in principal axes
    void PCA(ve::vec3f & vPriComp0, ve::vec3f & vPriComp1,
             float & minExtend0, float & maxExtend0,
             float & minExtend1, float & maxExtend1,
             const visiGraph & graph, unsigned int id) const;
    /// computes a polygonal isovist and stores control vertices in vPolygon
    /** \param center is the position for the isovist's center
     \param vBoundary holds the model geometry
     \param resolution defines the delta angle between the casted rays.
     \param tolerance defines the delta angle between ajacent vertices that is collapsed 
     \param ang0 (optional) start angle of a partial isovist
     \param ang1 (optional) end angle of a partial isovist
      \return a vector of boundary vertices
     */
    static std::vector<ve::vec3f> isovist(const ve::vec3f & center,
                 const std::vector<ve::triangle> & vBoundary,
                 float resolution, float tolerance, float ang0=0.0f, float ang1=0.0f);
    /// returns the ratio between visibly parts and invisible parts at a given fistance.
    /** \param position is the physical coordinates of the testpoint.
     \param vBoundary holds the model geometry.
     \param resolution defines the delta angle between the casted rays. */
    float visRatio(float distance, const ve::vec3f & pos,
                   const std::vector<ve::triangle> & vBoundary,
                   float resolution) const;
	/// swaps
protected:
    /// stores characteristic values, results of analytic computations
    std::vector<float> m_vVal;
	/// stores which value is currently displayed
	unsigned int m_currVal;
    /// stores connections to other vertices
    std::vector<bool> m_vEdge;
};

//--- class visiGraph ----------------------------------------------- /*FOLD00*/
enum { VGR_STATE_NONE=0,
       VGR_STATE_POSITION,
       VGR_STATE_ADDREMOVE,
       VGR_STATE_COMPUTE,
       VGR_STATE_CLUSTERING,
       VGR_STATE_NBH2,
       VGR_STATE_NVTX,
       VGR_STATE_COMPUTATION_DONE,
       VGR_STATE_READY,
       VGR_STATE_ISOVIST1,
       VGR_STATE_ISOVIST2,
       VGR_STATE_REVELATION1,
       VGR_STATE_PERIMETER,
       VGR_STATE_CLUSTERING1,
       VGR_STATE_MEASURANDS,
       VGR_STATE_PCA,
       VGR_STATE_CIRCUM2,
       VGR_STATE_CIRCUM4,
       VGR_STATE_PROFILE,
       VGR_STATE_PARTIAL
};

/// a class for visibility graph analysis
class visiGraph : public ve::geoNode {
public:
    /// default constructor
    visiGraph();
    /// constructor interpreting an initialization file
    visiGraph(ve::xmlIni & ini);
    /// destructor
    virtual ~visiGraph();
	/// fake copy operator
	virtual ve::geoNode * copy(bool copyGeometry=false) { return 0; }
    /// draws object
    virtual void draw();
    /// resets graph to initial state
    void clear();
    /// starts an interactive generation process
    void generate(const ve::geoNode & permeability);
    /// sets boundary model in the interactive process
    void boundaryModel(const ve::geoNode & boundary, const std::string & name="");
    /// returns boundary model geometry
    const std::vector<ve::triangle> & boundary() const { return vBoundary; };
    /// returns graph resolution
    float resolution() const { return m_res; };
    /// allows access to graph resolution
    float & resolution() { return m_res; };
    /// sets graph origin
    void origin(float newX, float newY, float newZ ) { orig.set(newX,newY,newZ); };
    /// returns graph origin
    const ve::vec3f & origin() const { return orig; };
    /// allows modification of graph origin
    ve::vec3f & origin() { return orig; };
    /// returns whether transparent walls are ignored during graph calculation
    bool ignoreTranspWalls() const { return m_ignoreTransp; };
    /// allows to change whether transparent walls are ignored during graph calculation
    bool & ignoreTranspWalls() { return m_ignoreTransp; };
    /// returns state
    unsigned int state() const { return m_state; };
    /// sets state
    void state(unsigned int newState) { m_state=newState; };
    /// switches to next step, make computation or switch state
    void nextStep();
    /// returns reference to graph vertex n
    const graphNode & operator[](unsigned int n) const { return m_vNode[n]; };
    /// adds or removes a cell from the view graph;
    void switchCell(const ve::line & ray);
    /// returns graph as string
    std::string str() const;
    /// returns boundary model name
    const std::string & modelName() const { return boundaryModelName; };
    /// loads visibility graph
    int load(const std::string & fileName);
    /// saves visibility graph
    int save(const std::string & fileName);
    /// saves current analysis
    int saveAnalysis(const std::string & fileName) const;
    /// returns current message
    std::string message() { std::string s=m_msg; m_msg=""; return s; };
    /// returns current progress value.
    /** The progress in long computations can be displayed this way.
     Values normally lie between 0.0 and 1.0. A Value < 0.0 indicates
     that now progress info is currently available. */
    float progress() const { return m_progress; };

    /// returns whether graph is currently displayed
    bool isShown() const { return isDisplayed; };
    /// sets whether graph is currently displayed
    void isShown(bool yesno) { isDisplayed=yesno; };
    /// returns whether graph is displayed in color
    bool color() const { return m_bColor; }
    /// sets whether graph is displayed in color
    void color(bool yesno) { m_bColor=yesno; }

    /// returns graph's X size
    unsigned int sizeX() const { return szX; };
    /// returns graph's Y size
    unsigned int sizeY() const { return szY; };
    /// returns graph's size
    unsigned int size() const { return m_vNode.size(); };
    /// returns value under the mouse pointer
    float value(const ve::line & ray);
    /// returns value at physical coordinate x|y
    float value(float x, float y);
    /// returns all measurands under the mouse pointer
    std::string measurands(const ve::line & ray);
    /// returns the isovist profile as polar distances
    std::string profile(const ve::line & ray, float resolution=1.0f, float ang0=0.0f, float ang1=0.0f);
    /// returns basic measurands of a partial isovist
    std::string partial(const ve::line & ray, float resolution=1.0f, float ang0=0.0f, float ang1=0.0f);

    /// computes neighborhood size
    void computeNeighborhood();
    /// computes revelation by 1 step in every cardinal direction and stores it in value
    void computeRevelation1();
    /// shows a first order isovist, optionally define angular range
    void showIsovist(const ve::line & ray, float ang0=0.0f, float ang1=0.0f);
    /// shows a second order isovist
    void showIsovist2(const ve::line & ray);
    /// shows revelation by 1 step in every cardinal direction
    void showRevelation1(const ve::line & ray);
    /// shows clustering from one vertex
    void showClustering(const ve::line & ray);
    /// shows the perimeter around a vertex
    void showPerimeter(const ve::line & ray, float ang0=0.0f, float ang1=0.0f);
    /// computes the perimeter length for all vertices
    void computePerimeterLength();
    /// computes the isovist perimeter length / area for all vertices
    void computePerimeterDivArea();
    /// computes the ratio between open and closed isovist perimeter boundaries for all vertices
    void computePerimeterOpenness();
    /// shows principal components of a first order isovist
    void showPCA(const ve::line & ray);
    /// shows visible and invisible vertices at distance radius
    void showCircum(const ve::line & ray, float radius);
    /// calculates ratio of visible and invisible vertices at distance radius
    void calcVisRatio(float radius);
    /// calculates minimum wall distance
    void calcMinDist();
	/// calculates minima or maxima of current graph derivative
	void calcExtrema(bool testMinima);
    /// returns last intersection coordinate
    const ve::vec3f & lastIntersection() { return m_lastIntersection; };

    /// sets start angle of partial isovists
    void angleStart(float a) { m_ang0=a; }
    /// sets end angle of partial isovists
    void angleEnd(float a) { m_ang1=a; }
    /// returns start angle of partial isovists
    float angleStart() const { return m_ang0; }
    /// returns end angle of partial isovists
    float angleEnd() const { return m_ang1; }
protected:
    /// tests for an intersection between a line and the graph.
    /**
     \param ray the line to be tested
     \param i graph X coordinate of intersection
     \param j graph X coordinate of intersection
     \return true in case of an intersection, otherwise false */
    bool intersects(const ve::line & ray, unsigned int & i, unsigned int & j);
    /// updates m_valueMin and m_valueMax
    void updateRange();

    /// stores origin of graph
    ve::vec3f orig;
    /// stores x size
    unsigned int szX;
    /// stores y size
    unsigned int szY;
    /// stores graph resolution
    float m_res;
    /// stores whether transparent walls are ignored during graph calculation
    bool m_ignoreTransp;
    /// stores perimeter resolution
    float periResolution;
    /// stores perimeter angle tolerance
    /** vertices with smaller angle differences may be discarded. */
    float periAngleTolerance;
    /// stores graph nodes
    std::vector<graphNode> m_vNode;

    /// stores state of view graph
    unsigned int m_state;
    /// counter variable for non-blocking operations
    unsigned int m_step;
    /// stores boundary geometry
    std::vector<ve::triangle> vBoundary;
    /// stores permeability geometry
    std::vector<ve::triangle> vPermeability;
    /// stores current minimum value
    float m_valueMin;
    /// stores current maximum value
    float m_valueMax;
    /// stores minimum coordinate
    ve::vec3f minCoord;
    /// stores maximum coordinate
    ve::vec3f maxCoord;
    /// stores last intersection vertex
    ve::vec3f m_lastIntersection;
    /// stores current message
    std::string m_msg;
    /// stores current progress value
    float m_progress;
    /// stores graph file name
    std::string m_graphFileName;
    /// stores boundary model name
    std::string boundaryModelName;
    
    /// start angle of partial isovists, experimental feature!
    float m_ang0;
    /// end angle of partial isovists
    float m_ang1;

    /// stores graph vertices
    std::vector<ve::vec3f> vVertex;
    /// stores whether graph is currently displayed
    bool isDisplayed;
    /// stores whether graph is displayed in color
    bool m_bColor;

    // values for showing the PCA, probably temporary:
    /// current analysed vertex
    ve::vec3f standPoint;
    /// center of gravity of viewshed
    ve::vec3f centroid;
    /// eigenvectors of PCA of viewshed
    ve::vec3f vEig[2];
    /// maximum extends of viewshed from standPoint in eigenvector coordinates
    float minExtend0, maxExtend0, minExtend1, maxExtend1;

    /// stores boundary vertices
    std::vector<ve::vec3f> vPolyBoundary;
};


#endif // _VISIGRAPH_H
