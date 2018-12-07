#ifndef _FL_AUX_H
#define _FL_AUX_H

#include <veStd.h>
#include <veMath.h>

//--- class deviceGraphicsFltk -------------------------------------
#include <FL/Fl_Gl_Window.H>

// forward declarations
namespace ve {
    class xmlIni;
    class geoNode;
};

/** \brief a class for showing ve::geoObjects in an FLTK based
 OpenGL window.
 version 0.4.0 (c) 2004-03-25 by Gerald.Franz@tuebingen.mpg.de \n
 started 2002-01-25
 */
class deviceGraphicsFltk : public Fl_Gl_Window {
public:
    /// constructor
    deviceGraphicsFltk( int x, int y, int b, int h, ve::xmlIni & ini, const char* c=0);
    /// initializes OpenGL, must be called after window is prepared
    void initGraphics();
    /// draws current scene
    virtual void draw();
    /// event handler
    int handle(int e);
    /// handles resize events
    virtual void resize(int x, int y, int w, int h);
    /// adds a model instance to the scene.
    virtual void addNode(ve::geoNode * glObject);
    ///  removes a model from the scene.
    virtual void dropNode(ve::geoNode * glObject);
    /// removes all children
    virtual void clear( bool isDelete=false );
    /// checks for user input events, ijn this case mouse and keyboard events
    void getInput(ve::vec6f & axis, unsigned int & inputState );
    /// changes projection type
    void flipProjection() { isPerspect=!isPerspect; invalidate(); };
    /// returns true if projection type is perspective
    bool isPerspective() const { return isPerspect; };
    /// returns world coordinate of p(relX|relY) on the current projection plane
    const ve::vec3f worldCoords(float relX, float relY) const;
    /// returns a ray in the direction of the last mouse click
    const ve::line mouseRay() const;
    /// allows access to the children vector
    ve::geoNode* node(unsigned int n) { return children[n]; };
    /// returns number of children
    unsigned int size() const { return children.size(); };

    /// controls view
    ve::vec6f & camera() { return observerPos; };
protected:
    /// vector for pointers to glObj model classes
    std::vector<ve::geoNode*> children;
    /// stores glClearColor
    float bgColor[3];
    /// stores input axes
    ve::vec6f inputAxis;
    /// stores state, mainly of mouse buttons
    unsigned int state;
    /// stores whether projection is perspective or axonometric
    bool isPerspect;
    /// stores camera rotation speed
    float camRotSpeed;
    /// stores camera translation speed
    float camTranslSpeed;

    /// left frustum clipping distance.
    float frustumLeft;
    /// right frustum clipping distance.
    float frustumRight;
    /// bottom frustum clipping distance.
    float frustumBottom;
    /// top frustum clipping distance.
    float frustumTop;
    /// distance to near clipping plane.
    float nearClipping;
    /// distance to far clipping plane.
    float farClipping;

    /// stores camera position
    ve::vec6f observerPos;
};


//--- class deviceWindowFltk ---------------------------------------
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>

/// an application main window class providing a menu and a status bar
class deviceWindowFltk : public Fl_Window {
public:
    /// constructor
    deviceWindowFltk(ve::xmlIni & ini, int argc, char **argv);
    /// destructor
    virtual ~deviceWindowFltk();
    /// sets left info text
    void infoL(const std::string & s) {
        pInfoLeft->value(s.c_str()); }
    /// sets right info text
    void infoR(const std::string & s) {
        if(!pProgress->visible()) pInfoRight->value(s.c_str()); }
    /// returns user input text only once
    std::string inputText() {
        std::string ret=m_userInput; m_userInput=""; return ret; }
    /// sets visual
    void visual(deviceGraphicsFltk * getVis) {
        pVisual=getVis; add_resizable(*getVis); }
    /// returns pointer to current visual
    deviceGraphicsFltk * visual() { return pVisual; }
    /// sets menu entries
    void menu(Fl_Menu_Item * entries) {
        pMenu->menu(entries); pMenu->redraw(); }
    /// returns reference to menu
    Fl_Menu_Bar & menu() { return *pMenu; }
    /// returns default text size
    int textSize() const { return textSz; }
    /// shows the progress bar in the info line
    void showProgress(float minVal=0.0f, float maxVal=1.0f );
    /// shows the right info text in the info line
    void showInfoR();
    /// shows an Ok and a cancel button in the info line
    void showOkCancel();
    /// returns progress value
    float progress() const {
        if(pProgress) return pProgress->value(); else return 0; }
    /// sets progress value
    void progress(float f) { if(pProgress) pProgress->value(f); }
    /// returns a stop event
    bool stop() {
        if(stopRequested) { stopRequested=false; return true; } return false; }
    /// returns an ok event
    bool buttonOk() {
        if(okRequested) { okRequested=false; return true; } return false; }
protected:
    /// callback method for exit calls
    static void cbExit(Fl_Widget* w, void* data );
    /// callback method for buttons
    static void cbButton(Fl_Widget* w, void* data );
    /// callback method for user input
    static void cbInput(Fl_Widget* w, void* data );

    /// stores pointer to current visualization class
    deviceGraphicsFltk * pVisual;
    /// pointer to left infoline label
    Fl_Output * pInfoLeft;
    /// pointer to right infoline label
    Fl_Input * pInfoRight;
    /// pointer to menu
    Fl_Menu_Bar * pMenu;
    /// pointer to progress bar
    Fl_Progress * pProgress;
    /// pointer to cancel button
    Fl_Button * pButtonCancel;
    /// pointer to ok button
    Fl_Button * pButtonOk;

    /// stores default text size
    int textSz;
    /// stores whether a stop event has occured
    bool stopRequested;
    /// stores whether an ok event has occured
    bool okRequested;
    /// stores user input text
    std::string m_userInput;
};

//--- class flDialog -----------------------------------------------
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>

/** \brief a generic dynamic input dialog class.
 version 0.1.1\n
 (c)2003-09-04 by Gerald.Franz@tuebingen.mpg.de\n
 started 2002-09-03
 */
class flDialog {
public:
    /// constructor
    flDialog(int width, const std::string & title="", int textHeight=12 );
    /// destructor
    ~flDialog();
    /// clears all inputs
    void clear();
    /// adds a float input
    void add(float & value, const char * label, float deflt=0.0f, bool separator=false,
             float step=0.0f, float fMin=0.0f, float fMax=1.0f);
    /// adds a bool input
    void add(bool & value, const char * label, bool deflt=false, bool separator=false);
    /// updates dialog, shown variable values are checked for consistency with memory values.
    void update();
protected:
    /// generic callback
    void callback(Fl_Widget* pWidget);
    /// static callback translator
    static void sCallback(Fl_Widget* pWidget, void* data) { 
        ((flDialog*)data)->callback(pWidget); }

    /// dialog window
    Fl_Window        *pDialogWindow;

    /// stores standard text height
    int textSize;
    /// stores dialog pixel width
    int w;
    /// stores widget standard height
    int widgetH;
    /// stores pointers to data variables
    std::vector<int> pData;
};

//--- class flSplash -----------------------------------------------
/// a class for displaying a splash screen
class flSplash {
public:
    /// constructor
    flSplash(const std::string & imgName, const std::string & msg, int x, int y);
    /// destructor
    ~flSplash() { pWnd->hide(); }
protected:
    /// stores window pointer
    Fl_Window * pWnd;
};

//--- class flConsole ----------------------------------------------

#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

/// a class for text output
class flConsole : public Fl_Window {
public:
    /// constructor 
    flConsole(unsigned int sizeX=640, unsigned int sizeY=480, const std::string & title="output window");
    /// destructor
    ~flConsole() { hide(); }
    /// clears console
    void clear();
    /// returns the number pof the currently stored characters
    unsigned int size() const { return pBuffer->length(); }
    /// adds a text
    void operator<<(const std::string & s) { pBuffer->append(s.c_str()); }
protected:
    /// callback for menu input
    void cbMenu(Fl_Widget* w);
    /// static callback translator
    static void s_cbMenu(Fl_Widget* pWidget, void* data) { 
        ((flConsole*)data)->cbMenu(pWidget); }
        
    /// pointer to menu
    Fl_Menu_Bar * pMenu;
    /// text widget
    Fl_Text_Display *pOutput;
    /// text manager
    Fl_Text_Buffer *pBuffer;
};

#endif // _FL_AUX_H
