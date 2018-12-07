#include "flAux.h"

#include <veTypes.h>
#include <veXml.h>
#include <veGeoObj.h>

#include <GL/glu.h>

#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Box.H>

#include <iostream>
#include <typeinfo>

using namespace std;
using namespace ve;

//--- class deviceWindowFltk ---------------------------------------

deviceWindowFltk::deviceWindowFltk(xmlIni & ini, int argc, char **argv) : Fl_Window(0,0) {
    int winSizeX, winSizeY;
    string winTitle,flScheme;
    ini.read(winSizeX,     "winSizeX", 640);
    ini.read(winSizeY,     "winSizeY", 480);
    ini.read(winTitle,     "winTitle","");
    label(winTitle.c_str());
    ini.read(textSz,       "textSize", 12);
    ini.read(flScheme,     "flScheme","");
    bool fullScreen;
    ini.read(fullScreen,   "fullScreen",false);
    if(fullScreen) fullscreen();
    else size(winSizeX,winSizeY);
    winSizeX=w();
    winSizeY=h();
    Fl::scheme(flScheme.c_str());

    pInfoLeft = new Fl_Output(0,winSizeY-20,winSizeX-121,20);
    pInfoLeft->textsize(textSz);
    pInfoLeft->box(FL_THIN_UP_BOX);
    pInfoLeft->color(FL_BACKGROUND_COLOR);
    pInfoRight = new Fl_Input(winSizeX-120,winSizeY-20,120,20);
    pInfoRight->textsize(textSz);
    pInfoRight->box(FL_THIN_UP_BOX);
    pInfoRight->color(FL_BACKGROUND_COLOR);
    pInfoRight->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);
    pInfoRight->callback((Fl_Callback*)cbInput, this);

    pProgress = new Fl_Progress(pInfoRight->x(), pInfoRight->y(),
                                pInfoRight->w()-pInfoRight->h()-1, pInfoRight->h());
    pProgress->color(FL_BACKGROUND_COLOR);
    pProgress->color2(fl_rgb_color(0,150,0));
    pProgress->box(FL_THIN_UP_BOX);
    pProgress->hide();

    pButtonOk = new Fl_Button(pInfoRight->x(), pInfoRight->y(),
                              pInfoRight->w()-pInfoRight->h()-1, pInfoRight->h(),"OK");
    pButtonOk->labelfont(FL_HELVETICA_BOLD);
    pButtonOk->labelcolor(fl_rgb_color(0,150,0));
    pButtonOk->box(FL_THIN_UP_BOX);
    pButtonOk->tooltip("confirm/continue operation");
    pButtonOk->hide();
    pButtonOk->callback((Fl_Callback*)cbButton, this);
    okRequested=false;

    pButtonCancel = new Fl_Button(winSizeX-pInfoRight->h(), pInfoRight->y(),
                                pInfoRight->h(), pInfoRight->h(),"@#-21+");
    pButtonCancel->labelcolor(FL_RED);
    pButtonCancel->box(FL_THIN_UP_BOX);
    pButtonCancel->tooltip("cancel operation");
    pButtonCancel->hide();
    pButtonCancel->callback((Fl_Callback*)cbButton, this);
    stopRequested=false;

    pMenu=new Fl_Menu_Bar(0,0,winSizeX-21*fullScreen,20);
    pMenu->box(FL_THIN_UP_BOX);
    if(fullScreen) {
        Fl_Button * pButtonExit=new Fl_Button(winSizeX-20,0,20,20,"@#-21+");
        pButtonExit->callback((Fl_Callback*)cbExit, (void*)"exit");
        pButtonExit->box(FL_THIN_UP_BOX);
        pButtonExit->tooltip("quit");
    }

    resizable(this);
    size_range(256,128);
    end();
    show(argc, argv);
}

deviceWindowFltk::~deviceWindowFltk() {
    delete pInfoLeft;
    delete pInfoRight;
    delete pMenu;
    delete pProgress;
    delete pButtonCancel;
    delete pButtonOk;
}

void deviceWindowFltk::showProgress(float minVal, float maxVal ) {
    if(!pProgress->visible()) {
        pInfoRight->hide();
        pProgress->show();
        pButtonCancel->show();
        pButtonOk->hide();
    }
    pProgress->minimum(minVal);
    pProgress->maximum(maxVal);
}

void deviceWindowFltk::showInfoR() {
    if(pInfoRight->visible()) return;
    pProgress->hide();
    pButtonCancel->hide();
    pButtonOk->hide();
    pInfoRight->show();
}

void deviceWindowFltk::showOkCancel() {
    if(pButtonOk->visible()) return;
    pProgress->hide();
    pInfoRight->hide();
    pButtonCancel->show();
    pButtonOk->show();
}

void deviceWindowFltk::cbExit(Fl_Widget* w, void* ) {
    w->window()->hide();
}

void deviceWindowFltk::cbButton(Fl_Widget* w, void* data ) {
    if(strcmp(w->label(),"OK")==0)
        ((deviceWindowFltk*)data)->okRequested=true;
    else ((deviceWindowFltk*)data)->stopRequested=true;
}

void deviceWindowFltk::cbInput(Fl_Widget* w, void* data ) {
    ((deviceWindowFltk*)data)->m_userInput=((deviceWindowFltk*)data)->pInfoRight->value();
}


//--- class deviceGraphicsFltk -------------------------------------

deviceGraphicsFltk::deviceGraphicsFltk( int x, int y, int b, int h, xmlIni & ini, const char* c)
: Fl_Gl_Window(x,y,b,h,c) {
    // 3d viewport initialization:
    if(ini.subTag("deviceGraphics")) {
        ini.focusOn("deviceGraphics");
        ini.read(frustumLeft,  "frustumLeft",   -1.0f);
        ini.read(frustumRight, "frustumRight",   1.0f);
        ini.read(frustumBottom,"frustumBottom", -0.75f);
        ini.read(frustumTop,   "frustumTop",     0.75f);
        ini.read(nearClipping, "nearClipping",   1.0f);
        ini.read(farClipping,  "farClipping",  100.0f);
        if(ini.subTag("bool","backFaceCulling")) {
            bool isCulling;
            ini.read(isCulling,"backFaceCulling",false);
            if(isCulling) {
                glCullFace(GL_BACK);
                glEnable(GL_CULL_FACE); // should maybe called each frame, but currently it works this way identical
            }
        }
        ini.focusOff();
    }
    else {
        frustumLeft=-1.0;
        frustumRight=1.0f;
        frustumBottom=-0.75;
        frustumTop=0.75;
        nearClipping=0.1f;
        farClipping=100.0f;
    }
    // adjust frustum to clipping distance:
    frustumLeft*=nearClipping;
    frustumRight*=nearClipping;
    frustumBottom*=nearClipping;
    frustumTop*=nearClipping;

    ini.focusOn("deviceGraphics");
    ini.read(camTranslSpeed,"camTranslSpeed",5);
    ini.read(camRotSpeed,"camRotSpeed",15);
    if(ini.subTag("camera","0")) // read camera start position
        observerPos.set(ini.subTag("camera","0")->getAttribute("pos"));
    ini.focusOff();

    ini.read(bgColor,3,"bgColor");
    state=0;
    isPerspect=false;
}

void deviceGraphicsFltk::initGraphics() {
    show();
    redraw();
    Fl::wait(0.0); // update fltk window
}

void deviceGraphicsFltk::draw() {
    if (!valid()||!isPerspect) { // OpenGL initialization
        glViewport(0,0,w(),h());
        glClearColor( bgColor[0], bgColor[1], bgColor[2], 1.0f);

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();

        float stretch=(float(w())/float(h()))/((frustumRight-frustumLeft)/(frustumTop-frustumBottom)); // adjust to actual size
        frustumLeft*=stretch;
        frustumRight*=stretch;
        if(isPerspect) glFrustum(frustumLeft,frustumRight,
                                 frustumBottom,frustumTop,
                                 nearClipping,farClipping);
        else glOrtho(frustumLeft*fabs(observerPos[Z]),frustumRight*fabs(observerPos[Z]),
                     frustumBottom*fabs(observerPos[Z]),frustumTop*fabs(observerPos[Z]),
                     nearClipping,farClipping);

        glMatrixMode( GL_MODELVIEW );
    }

    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(-observerPos[R],0,1,0);
    glRotatef(-observerPos[P]-90,1,0,0);
    glRotatef(-observerPos[H],0,0,1);
    glTranslatef(-observerPos[X],-observerPos[Y],-observerPos[Z]);

    for(vector<ve::geoNode*>::iterator i=children.begin(); i!=children.end(); i++)
        (*i)->draw();
    glPopMatrix();
}

void deviceGraphicsFltk::resize(int x, int y, int w, int h) {
    Fl_Gl_Window::resize(x,y,w,h);
    invalidate();
}


void deviceGraphicsFltk::addNode(ve::geoNode * veGeoObject) {
    if(veGeoObject==NULL) return;
    children.push_back(veGeoObject);
    veGeoObject->initGraphics();
}

void deviceGraphicsFltk::dropNode(ve::geoNode * veGeoObject) {
    for(vector<ve::geoNode*>::iterator i=children.begin(); i!=children.end(); i++)
        if((*i)==(veGeoObject)) {
            children.erase(i);
            return;
        }
}

void deviceGraphicsFltk::clear(bool isDelete) {
    if(isDelete) for(vector<ve::geoNode*>::iterator i=children.begin(); i<children.end(); i++)
        delete (*i);
    children.clear();
}

int deviceGraphicsFltk::handle(int e) {
    switch(e) {
    case FL_PUSH:
        if(Fl::event_button()==FL_LEFT_MOUSE)        state|=BIT[BUTTON_0];
        else if(Fl::event_button()==FL_RIGHT_MOUSE)  state|=BIT[BUTTON_1];
        else if(Fl::event_button()==FL_MIDDLE_MOUSE) state|=BIT[BUTTON_2];
        inputAxis[H]=(float(Fl::event_x())/w()-0.5f)*-2.0f;
        inputAxis[P]=(float(Fl::event_y())/h()-0.5f)*-2.0f;
        return 1;
    case FL_MOUSEWHEEL:
        inputAxis[R]=Fl::event_dy();
        return 1;
    case FL_DRAG:
        inputAxis[H]=(float(Fl::event_x())/w()-0.5f)*-2.0f;
        inputAxis[P]=(float(Fl::event_y())/h()-0.5f)*-2.0f;
        return 1;
    case FL_RELEASE:
        if((Fl::event_button()==FL_LEFT_MOUSE)&&(state&BIT[BUTTON_0]))
            state-=BIT[BUTTON_0];
        else if((Fl::event_button()==FL_RIGHT_MOUSE)&&(state&BIT[BUTTON_1]))
            state-=BIT[BUTTON_1];
        else if((Fl::event_button()==FL_MIDDLE_MOUSE)&&(state&BIT[BUTTON_2]))
            state-=BIT[BUTTON_2];
        inputAxis[H]=(float(Fl::event_x())/w()-0.5f)*-2.0f;
        inputAxis[P]=(float(Fl::event_y())/h()-0.5f)*-2.0f;
        return 1;
    case FL_KEYDOWN:{
        switch(Fl::event_key()) {
        case FL_Left:
            inputAxis[X]=-1.0f;
            break;
        case FL_Right:
            inputAxis[X]=1.0f;
            break;
        case FL_Up:
            inputAxis[Y]=1.0f;
            break;
        case FL_Down:
            inputAxis[Y]=-1.0f;
            break;
        case FL_Page_Up:
            inputAxis[Z]=1.0f;
            break;
        case FL_Page_Down:
            inputAxis[Z]=-1.0f;
            break;
        case FL_Escape:
            state|=BIT[EVT_TERM];
            break;
        }
        return 1;
    }
    case FL_KEYUP:{
        switch(Fl::event_key()) {
        case FL_Left:
        case FL_Right:
            inputAxis[X]=0.0f;
            break;
        case FL_Up:
        case FL_Down:
            inputAxis[Y]=0.0f;
            break;
        case FL_Page_Up:
        case FL_Page_Down:
            inputAxis[Z]=0.0f;
            break;
        case FL_Escape:
            if(state&BIT[EVT_TERM]) state-=BIT[EVT_TERM];
            break;
        }
        return 1;
    }
    case FL_FOCUS:
    case FL_UNFOCUS:
        return 1;
    default:
        return 0;
    }
}

void deviceGraphicsFltk::getInput(vec6f & axis, unsigned int & inputState ) {
    axis=inputAxis;
    inputAxis[R]=0;
    inputState=state;
}

const vec3f deviceGraphicsFltk::worldCoords(float relX, float relY) const {
    if(isPerspect) {
        // translate according to actual frustum:
        relX=frustumLeft+((relX+1.0f)*0.5f)*(frustumRight-frustumLeft);
        relY=frustumBottom+((relY+1.0f)*0.5f)*(frustumTop-frustumBottom);
        // compute direction vectors:
        vec3f v[3];
        const int VJU=0; const int HOR=1; const int VER=2;
        v[VJU].set(0,1,0);
        v[HOR].set(1,0,0);
        v[VER].set(0,0,1);
        for(unsigned int i=0; i<3; i++) {
            v[i].rotate(observerPos[R], vec3f(0,1,0));
            v[i].rotate(observerPos[P], vec3f(1,0,0));
            v[i].rotate(observerPos[H], vec3f(0,0,1));
            v[i]*=nearClipping;
        }
        return observerPos+v[VJU]+(v[HOR]*relX)+(v[VER]*relY);
    }
    // translate according to actual frustum:
    relX=(frustumLeft+((relX+1.0f)*0.5f)*(frustumRight-frustumLeft))*fabs(observerPos[Z]);
    relY=(frustumBottom+((relY+1.0f)*0.5f)*(frustumTop-frustumBottom))*fabs(observerPos[Z]);
    // compute direction vectors:
    vec3f v[3];
    const int VJU=0; const int HOR=1; const int VER=2;
    v[VJU].set(0,1,0);
    v[HOR].set(1,0,0);
    v[VER].set(0,0,1);
    for(unsigned int i=0; i<3; i++) {
        v[i].rotate(observerPos[R], vec3f(0,1,0));
        v[i].rotate(observerPos[P], vec3f(1,0,0));
        v[i].rotate(observerPos[H], vec3f(0,0,1));
        //v[i]*=nearClipping;
    }
    return observerPos+v[VJU]+(v[HOR]*relX)+(v[VER]*relY);
}

const line deviceGraphicsFltk::mouseRay() const {
    if(!isPerspect) {
        vec3f vVju(0,1,0);
        vVju.rotate(observerPos[R], vec3f(0,1,0));
        vVju.rotate(observerPos[P], vec3f(1,0,0));
        vVju.rotate(observerPos[H], vec3f(0,0,1));
        vec3f secondVec(worldCoords(-inputAxis[H],inputAxis[P]));
        secondVec.translate(vVju);
        return line(worldCoords(-inputAxis[H],inputAxis[P]),secondVec);
    }
    return line(observerPos,worldCoords(-inputAxis[H],inputAxis[P]));
}

//--- class flDialog -----------------------------------------------

flDialog::flDialog(int width, const string & title, int textHeight) {
    textSize=textHeight;
    w=width;
    widgetH=25;

    Fl_Window* o = pDialogWindow = new Fl_Window(w, 0, title.c_str());
    o->user_data((void*)(this));
    o->callback((Fl_Callback*)sCallback,this);
    o->set_non_modal();
    o->end();
    o->show();
}

flDialog::~flDialog() {
    clear();
    pDialogWindow->hide();
    delete pDialogWindow;
}

void flDialog::clear() {
    pData.clear();
    for(int i=2; i<pDialogWindow->children(); i++) {
        Fl_Widget* pWidget=pDialogWindow->child(i);
        pDialogWindow->remove(pDialogWindow->child(i));
        delete pWidget;
    }
    pDialogWindow->size(w,widgetH);
    pDialogWindow->redraw();
}

void flDialog::add(float & value, const char * label, float deflt, bool separator,
                   float step, float fMin, float fMax) {
    value=deflt;
    pData.push_back(int(&value));
    if(separator) pDialogWindow->size(w,pDialogWindow->h()+widgetH+12);
    else pDialogWindow->size(w,pDialogWindow->h()+widgetH);

    Fl_Value_Input* o = new Fl_Value_Input(w/2, pDialogWindow->h()-widgetH,
                                           w/2, widgetH, label);
    o->step(step);
    if(step) o->range(fMin,fMax);
    o->box(FL_THIN_DOWN_BOX);
    o->value(value);
    o->textsize(textSize);
    o->labelsize(textSize);
    o->callback((Fl_Callback*)sCallback,this);

    pDialogWindow->add(o);
    pDialogWindow->redraw();
}

void flDialog::add(bool & value, const char * label, bool deflt, bool separator) {
    value=deflt;
    pData.push_back(int(&value));
    if(separator) pDialogWindow->size(w,pDialogWindow->h()+widgetH+12);
    else pDialogWindow->size(w,pDialogWindow->h()+widgetH);

    Fl_Check_Button* o = new Fl_Check_Button(w/2, pDialogWindow->h()-widgetH,
                                             w/2, widgetH, label);
    o->align(FL_ALIGN_LEFT);
    o->value(value);
    o->labelsize(textSize);
    o->callback((Fl_Callback*)sCallback,this);

    pDialogWindow->add(o);
    pDialogWindow->redraw();
}

void flDialog::callback(Fl_Widget* pWidget) {
    if(pWidget->label()==pDialogWindow->label()) return; // don't handle close events
    else for(int i=0; i<pDialogWindow->children(); i++)
        if(string(pDialogWindow->child(i)->label())==string(pWidget->label())) {
            const type_info & T1=typeid(*pDialogWindow->child(i));
            static const type_info & Tfloat=typeid(Fl_Value_Input);
            static const type_info & Tbool=typeid(Fl_Check_Button);
            if(T1==Tfloat)
                *((float*)(pData[i]))=((Fl_Value_Input*)pWidget)->value();
            else if(T1==Tbool) {
                *((bool*)(pData[i]))=((Fl_Check_Button*)pWidget)->value();
            }
        }
}

void flDialog::update() {
    for(int i=0; i<pDialogWindow->children(); i++) {
        const type_info & T1=typeid(*pDialogWindow->child(i));
        static const type_info & Tfloat=typeid(Fl_Value_Input);
        static const type_info & Tbool=typeid(Fl_Check_Button);
        if(T1==Tfloat) {
            float & value=*((float*)(pData[i]));
            if(value!=((Fl_Value_Input*)pDialogWindow->child(i))->value())
                ((Fl_Value_Input*)pDialogWindow->child(i))->value(value);
        }
        else if(T1==Tbool) {
            bool & value=*((bool*)(pData[i]));
            if(value!=((Fl_Check_Button*)pDialogWindow->child(i))->value())
                ((Fl_Check_Button*)pDialogWindow->child(i))->value(value);
        }
    }
    pDialogWindow->redraw();
}



//--- class flSplash -----------------------------------------------
/// constructor
flSplash::flSplash(const std::string & imgName, const std::string & msg, int x, int y) {
    // show splash window:
    fl_register_images();
    Fl_Shared_Image * flSplash= Fl_Shared_Image::get(imgName.c_str());
    pWnd=new Fl_Window(flSplash->w(),flSplash->h()+12);
    Fl_Output * pOutp=new Fl_Output(0,flSplash->h(),flSplash->w(),12);
    pOutp->box(FL_FLAT_BOX);
    pOutp->value(msg.c_str());
    pOutp->textsize(10);
    pOutp->color(fl_rgb_color(127));
    Fl_Box * pBgBox = new Fl_Box(0,0,flSplash->w(),flSplash->h());
    pBgBox->image(flSplash);
    pBgBox->take_focus();
    pWnd->parent(0);
    pWnd->border(0);
    pWnd->box(FL_NO_BOX);    
    pWnd->position(x-pWnd->w()/2,y-pWnd->h()/2);
    pWnd->show();    
}

//--- class flConsole ----------------------------------------------

#include <FL/Fl_File_Chooser.H>

flConsole::flConsole(unsigned int winSizeX, unsigned int winSizeY, const std::string & title) : Fl_Window(winSizeX,winSizeY) {
    char * pTitle=new char[title.size()+1]; // allocate on the heap to let it survive
    strncpy(pTitle,title.c_str(),title.size());
    label(pTitle);
    
    pMenu=new Fl_Menu_Bar(0,0,winSizeX,20);
    pMenu->box(FL_THIN_UP_BOX);
    int textSize=12;
    static Fl_Menu_Item menuItem[] = {
        { "&Save As", 0, (Fl_Callback*)s_cbMenu, this, 0, FL_NORMAL_LABEL,0,textSize },
        { "&Copy", 0, (Fl_Callback*)s_cbMenu, this, 0, FL_NORMAL_LABEL,0,textSize },
        { "Clea&r", 0, (Fl_Callback*)s_cbMenu, this, 0, FL_NORMAL_LABEL,0,textSize },
        { "Clear &Line", 0, (Fl_Callback*)s_cbMenu, this, 0, FL_NORMAL_LABEL,0,textSize },
        { 0 }
    };
    pMenu->menu(menuItem);
    pMenu->redraw();
    
	pBuffer=new Fl_Text_Buffer;
	pOutput=new Fl_Text_Display(0,20,winSizeX,winSizeY-20);
	pOutput->buffer(pBuffer);
	pOutput->scrollbar_align(FL_ALIGN_RIGHT);
    pOutput->box(FL_FLAT_BOX);
	resizable(pOutput);
    
    parent(0);
    end();
    show();    
}

void flConsole::cbMenu(Fl_Widget* w) {
    string cmd=((Fl_Menu_*)w)->mvalue()->label();
    if(cmd=="&Save As") {
        char * fileName=fl_file_chooser("Save text output as...","*.txt",NULL);
        if(!fileName) return;
        Fl::wait(0.0f); // close file dialog

        ofstream file(fileName, std::ios::out );
        if (!file.good()) return;
        const char * pText= pBuffer->text();
        file << pText;
        file.close();        
        free((void*)pText);
    }
    else if(cmd=="&Copy") {
        const char * pText= pBuffer->text();
        Fl::copy(pText,pBuffer->length(),1);
        free((void*)pText);
    }
    else if(cmd=="Clea&r") {
        pBuffer->remove(0,pBuffer->length());
    }
    else if((cmd=="Clear &Line")) {
        if(pBuffer->length()<2) {
            pBuffer->remove(0,pBuffer->length());
            return;
        }
        int brPos;
        if(pBuffer->findchars_backward(pBuffer->length()-2,"\n",&brPos)) 
            pBuffer->remove(brPos+1,pBuffer->length());
        else pBuffer->remove(0,pBuffer->length());
    }
    else {
        (*this) << "ERROR: unknown command "+cmd+".\n";
    }
}

