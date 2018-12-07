// started 2001-11-11

#include <veConfig.h>
#include <veStd.h>
#include <veGeoObj.h>
#include <veImage.h>
#include <veTypes.h>
#include <veXml.h>
#include <veMath.h>
#include <veUtils.h>

#include <FL/fl_draw.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#ifndef WIN32
#  include <FL/Fl_Help_Dialog.H>
#endif

#include "veIoObj.h"
#include "visiGraph.h"
#include "flAux.h"
#include "gl2ps.h"

using namespace std;
using namespace ve;

//--- global variables ---------------------------------------------

deviceWindowFltk * pDevWindow=0;
deviceGraphicsFltk * pDevGraphics=0;
ve::geoNode * pBoundary=0;
ve::geoNode * pPermeability= 0;
visiGraph * pVgr=0;
flDialog * pGraphSettings=0;
flConsole * pConsole=0;
#ifndef WIN32
Fl_Help_Dialog* pHelpDialog=0;
#endif

string outputFileName;
bool showModel=true;

//--- functions ----------------------------------------------------

int plot(deviceGraphicsFltk * pDevGraphics, const std::string&fileName, bool isEPS) {
    int outputFormat= isEPS ? GL2PS_EPS : GL2PS_PDF;
    const string producer=cmdLine::name()+" "+cmdLine::version()+" (c) "+cmdLine::date()+" by "+cmdLine::author();
    FILE *fp=fopen(fileName.c_str(),"wb");
    int ret=gl2psBeginPage(fileName.c_str(),producer.c_str(),0,outputFormat,GL2PS_BSP_SORT,
                           GL2PS_BEST_ROOT|GL2PS_USE_CURRENT_VIEWPORT|GL2PS_OCCLUSION_CULL,
                           GL_RGBA,0,0, 0,0,0, 1024*1024, fp,0);
    pDevGraphics->draw();
    ret+=gl2psEndPage();
    fclose(fp);
    return ret;
}


void cbMenu(Fl_Widget* w, void* data ) {
    string cmd((char*)data);
    if(cmd=="exit")
        w->window()->hide();
    else if(cmd=="fileNew") {
        /// load permeability model:
        char * fileName=fl_file_chooser("Select permeability model","*.{wrl,WRL,3ds,3DS,x3d,X3D,obj,OBJ}",NULL);
        if(!fileName) {
            pDevWindow->infoL("A permeability (collision) model has not been selected.");
            return;
        }
        pVgr->clear();
        if(pPermeability) {
            pDevGraphics->dropNode(pPermeability);
            delete pPermeability;
        }
        if(pBoundary) {
            pDevGraphics->dropNode(pBoundary);
            delete pBoundary;
            pBoundary=0;
        }
        pPermeability=geoFileHandler::load(fileName);
        pDevGraphics->dropNode(pVgr);
        pDevGraphics->addNode(pPermeability);
        pDevGraphics->addNode(pVgr);
        pVgr->generate(*pPermeability);
        pGraphSettings= new flDialog(250,"Properties");
        pGraphSettings->add(pVgr->origin()[Z],"height over ground", pVgr->origin()[Z],false,.1f,-10,10);
        pGraphSettings->add(pVgr->resolution(), "graph resolution", pVgr->resolution(),false,.05f,0,5);
        pGraphSettings->add(pVgr->origin()[X],"X origin",pVgr->origin()[X],true,.05f,-10,10);
        pGraphSettings->add(pVgr->origin()[Y],"Y origin",pVgr->origin()[Y],false,.05f,-10,10);
        pGraphSettings->add(pVgr->ignoreTranspWalls(),"ignore transp. walls",pVgr->ignoreTranspWalls(),true);
        pDevWindow->showOkCancel();
        pDevWindow->label("New file - ajanachara");
    }
    else if(cmd=="helpAbout") {
        fl_message((cmdLine::name()+" v"+cmdLine::version()+ " (c) "+cmdLine::date()+" by Gerald.Franz@@tuebingen.mpg.de").c_str());
    }
    else if((cmd=="helpHelp")) {
#ifdef WIN32
        string cmd="start "+cmdLine::dir()+"index.html";
        system(cmd.c_str());
#else
        if(pHelpDialog) {
            pHelpDialog->load((cmdLine::dir()+"index.html").c_str());
            pHelpDialog->show();
        }
#endif
    }
    else if(cmd=="viewModel") {
        if(!pBoundary) return;
        showModel=!showModel;
        if(showModel) pDevGraphics->addNode(pBoundary);
        else pDevGraphics->dropNode(pBoundary);
    }
    else if(cmd=="viewGraph") {
        if(pVgr) pVgr->isShown(!pVgr->isShown());
    }
    else if(cmd=="viewColor") {
        if(pVgr) pVgr->color(!pVgr->color());
    }
    else if((cmd=="viewPerspective")&&pDevGraphics) {
        pDevGraphics->flipProjection();
    }
    else if((cmd=="viewConsole")&&pConsole) {
        pConsole->show();
    }
    else if(cmd=="exportTGA") {
        char * fileName=fl_file_chooser("Save screenshot as...","*.tga",NULL);
        if(!fileName) return;
        Fl::wait(0.1); // update fltk windows, close file dialog
        pDevGraphics->redraw();
        pDevWindow->redraw();
        Fl::wait(0.1); // update fltk windows
        glTexture::grabScreen(fileName,pDevGraphics->w(),pDevGraphics->h());
        pDevWindow->infoL(("Screenshot \""+string(fileName)+"\" saved.").c_str());
    }
    else if(pVgr) {
        if((cmd=="analyzeNeighbors")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->computeNeighborhood();
        else if((cmd=="analyzeNeighbors2")&&(pVgr->state()>=VGR_STATE_READY)) {
            pDevWindow->infoL("computing second order neighborhood size...");
            pVgr->state(VGR_STATE_NBH2);
        }
        else if((cmd=="analyzeClusterField")&&(pVgr->state()>=VGR_STATE_READY)) {
            pDevWindow->infoL("computing clustering coefficient...");
            pVgr->state(VGR_STATE_CLUSTERING);
        }
        else if((cmd=="analyzeRevelField1")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->computeRevelation1();
        else if((cmd=="analyzePeriLength")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->computePerimeterLength();
        else if((cmd=="analyzeNVertices")&&(pVgr->state()>=VGR_STATE_READY)) {
            pDevWindow->infoL("computing number of vertices of isovist polygon...");
            pVgr->state(VGR_STATE_NVTX);
        }
        else if((cmd=="analyzePeriArea")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->computePerimeterDivArea();
        else if((cmd=="analyzeOpenness")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->computePerimeterOpenness();
        else if((cmd=="analyzeVisRatio2")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->calcVisRatio(2.0f);
        else if((cmd=="analyzeVisRatio4")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->calcVisRatio(4.0f);
        else if((cmd=="analyzeMinDist")&&(pVgr->state()>=VGR_STATE_READY))
            pVgr->calcMinDist();

        else if((cmd=="analyzeIsovist1")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_ISOVIST1);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzeIsovist2")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_ISOVIST2);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzeRevelation1")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_REVELATION1);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzeClustering")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_CLUSTERING1);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzePerimeter")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_PERIMETER);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzeMeasurands")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_MEASURANDS);
            pDevWindow->infoL("Pick vertex to analyze");
        }
        else if((cmd=="analyzePCA")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_PCA);
            pDevWindow->infoL("PCA: Pick vertex to analyze");
        }
        else if((cmd=="analyzeNearSpace")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_CIRCUM2);
            pDevWindow->infoL("Near space: Pick vertex to analyze");
        }
        else if((cmd=="analyzeMediumSpace")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_CIRCUM4);
            pDevWindow->infoL("Medium space: Pick vertex to analyze");
        }
        else if((cmd=="analyzeProfile")&&(pVgr->state()>=VGR_STATE_READY)) {
            pVgr->state(VGR_STATE_PROFILE);
            pDevWindow->infoL("Profile: Pick vertex to analyze");
        }
        else if(cmd=="fileOpen") {
            char * fileName=fl_file_chooser("Load visibility graph","*.{vgz,vgr.gz}",NULL);
            if(!fileName) return;
            pDevWindow->label((string(fileName)+" - ajanachara").c_str());
            pDevWindow->infoL("Loading graph...");
            Fl::wait(0.0f); // close file dialog
            pVgr->load(fileName);
            pVgr->computeNeighborhood();
            if(showModel) pDevGraphics->dropNode(pBoundary);
            if(pBoundary) delete pBoundary;

            string filePath(fileName);
            filePath=filePath.substr(0,filePath.rfind("/")+1);

            pBoundary=geoFileHandler::load(filePath+pVgr->modelName());
            pVgr->boundaryModel(*pBoundary);
            if(showModel) pDevGraphics->addNode(pBoundary);
        }
        else if((cmd=="fileSaveAs")&&(pVgr->state()>=VGR_STATE_READY)) {
            char * fileName=fl_file_chooser("Save visibility graph as...","*.vgz",NULL);
            if(!fileName) return;
            pDevWindow->label((string(fileName)+" - ajanachara").c_str());
            pDevWindow->infoL("Saving graph...");
            Fl::wait(0.0f); // close file dialog
            if(pVgr->save(fileName)!=0)
                pDevWindow->infoL(("Saving graph \""+string(fileName)+"\" failed.").c_str());
            else pDevWindow->infoL(("Graph \""+string(fileName)+"\" saved.").c_str());
        }
        else if(((cmd=="exportEPS")||(cmd=="exportPDF"))&&(pVgr->state()>=VGR_STATE_READY)) {
            bool isEPS=(cmd=="exportEPS") ? true : false;
            char * fileName=fl_file_chooser("Export",isEPS ? "*.{ps,eps}":"*.pdf",NULL);
            if(!fileName) return;
            pDevWindow->infoL(("Exporting file \""+string(fileName)+"\" ...").c_str());
            Fl::wait(0.0f); // close file dialog
            if(plot(pDevGraphics,fileName,isEPS)!=0)
                pDevWindow->infoL(("Exporting \""+string(fileName)+"\" failed.").c_str());
            else pDevWindow->infoL(("Exporting \""+string(fileName)+"\" done.").c_str());
        }
        else if(cmd=="exportASCII") {
            char * fileName=fl_file_chooser("Save analysis as...","*.txt",NULL);
            if(!fileName) return;
            if(pVgr->saveAnalysis(fileName)!=0)
                pDevWindow->infoL(("Saving analysis \""+string(fileName)+"\" failed.").c_str());
            else pDevWindow->infoL(("Analysis \""+string(fileName)+"\" saved.").c_str());
        }
		else if(cmd=="metaMinima" && pVgr->state()>=VGR_STATE_READY ) {
			pVgr->calcExtrema(true);
		}
		else if(cmd=="metaMaxima" && pVgr->state()>=VGR_STATE_READY ) {
			pVgr->calcExtrema(false);
		}
        else if(cmd=="analyzePartialIsovist") {
            const char * ch=fl_input("Please enter start angle:",f2s(pVgr->angleStart()).c_str());
            if(!ch) return;
            pVgr->angleStart(angle(s2f(ch)));
            ch=0;
            ch=fl_input("Please enter end angle:",f2s(pVgr->angleEnd()).c_str());
            if(!ch) return;
            pVgr->angleEnd(angle(s2f(ch)));
            pVgr->state(VGR_STATE_PARTIAL);
            pDevWindow->infoL("Please enter reference point.");
        }
    }
}


//--- main ---------------------------------------------------------

int main(int argc, char **argv) {
    // command line option handling:
    cmdLine::author ("Gerald.Franz@tuebingen.mpg.de");
    cmdLine::version ("1.3.1");
    cmdLine::date ("2006-07-29");
    cmdLine::shortDescr ("interactive visibility graph analysis program");
    cmdLine::usage ("[-i(niFile.xml)]");
    cmdLine::interpret(argc,argv);
    string iniFileName= cmdLine::opt('i') ? cmdLine::optArg('i') : cmdLine::dir()+"ini.xml";
    xmlIni ini;
    ini.load(iniFileName);
    
    /// initilialize GUI:
    pDevWindow=new deviceWindowFltk(ini,argc,argv);
    pConsole=new flConsole(640,480,"output - ajanachara");
    pConsole->iconize();
    pDevWindow->infoL(cmdLine::name()+" v"+cmdLine::version()+ " (c) "+cmdLine::date()+" by "+cmdLine::author());
    int textSize=pDevWindow->textSize();

    Fl_Menu_Item menuItem[] = {
        { "&File", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "&New", 0, (Fl_Callback*)cbMenu, (void*)"fileNew", 0, FL_NORMAL_LABEL,0,textSize },
        { "&Open...", 0, (Fl_Callback*)cbMenu, (void*)"fileOpen", 0, FL_NORMAL_LABEL,0,textSize },
        { "&Save as...", 0, (Fl_Callback*)cbMenu, (void*)"fileSaveAs", FL_MENU_DIVIDER, FL_NORMAL_LABEL,0,textSize },
        { "Export &ASCII...", 0, (Fl_Callback*)cbMenu, (void*)"exportASCII", 0, FL_NORMAL_LABEL,0,textSize },
        { "Export &TGA...", 0, (Fl_Callback*)cbMenu, (void*)"exportTGA", 0, FL_NORMAL_LABEL,0,textSize },
        { "Export &EPS...", 0, (Fl_Callback*)cbMenu, (void*)"exportEPS", 0, FL_NORMAL_LABEL,0,textSize },
        { "Export &PDF...", 0, (Fl_Callback*)cbMenu, (void*)"exportPDF", FL_MENU_DIVIDER, FL_NORMAL_LABEL,0,textSize },
        { "&Quit", FL_CTRL+'q', (Fl_Callback*)cbMenu, (void*)"exit", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { "&View", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "&Model", 0, (Fl_Callback*)cbMenu, (void*)"viewModel", FL_MENU_TOGGLE|FL_MENU_VALUE, FL_NORMAL_LABEL,0,textSize },
        { "&Graph", 0, (Fl_Callback*)cbMenu, (void*)"viewGraph", FL_MENU_TOGGLE|FL_MENU_VALUE, FL_NORMAL_LABEL,0,textSize },
        { "&Colored graph", 0, (Fl_Callback*)cbMenu, (void*)"viewColor", FL_MENU_TOGGLE|FL_MENU_VALUE, FL_NORMAL_LABEL,0,textSize },
        { "&Perspective", 0, (Fl_Callback*)cbMenu, (void*)"viewPerspective", FL_MENU_TOGGLE|FL_MENU_DIVIDER, FL_NORMAL_LABEL,0,textSize },
        { "&Show output", 0, (Fl_Callback*)cbMenu, (void*)"viewConsole", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { "&Isovist", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "1st order isovist", 0, (Fl_Callback*)cbMenu, (void*)"analyzeIsovist1", 0, FL_NORMAL_LABEL,0,textSize },
        { "2nd order isovist", 0, (Fl_Callback*)cbMenu, (void*)"analyzeIsovist2", 0, FL_NORMAL_LABEL,0,textSize },
        { "Visual similarity", 0, (Fl_Callback*)cbMenu, (void*)"analyzeClustering", 0, FL_NORMAL_LABEL,0,textSize },
        { "Revelation by 1 step",0,(Fl_Callback*)cbMenu, (void*)"analyzeRevelation1", 0, FL_NORMAL_LABEL,0,textSize },
        { "Perimeter", 0, (Fl_Callback*)cbMenu, (void*)"analyzePerimeter", 0, FL_NORMAL_LABEL,0,textSize },
        { "PCA", 0, (Fl_Callback*)cbMenu, (void*)"analyzePCA", 0, FL_NORMAL_LABEL,0,textSize },
        { "Near space", 0, (Fl_Callback*)cbMenu, (void*)"analyzeNearSpace", 0, FL_NORMAL_LABEL,0,textSize },
        { "Medium space", 0, (Fl_Callback*)cbMenu, (void*)"analyzeMediumSpace", FL_MENU_DIVIDER, FL_NORMAL_LABEL,0,textSize },
        { "depth profile data",0, (Fl_Callback*)cbMenu, (void*)"analyzeProfile", 0, FL_NORMAL_LABEL,0,textSize },
        { "all measurands", 0, (Fl_Callback*)cbMenu, (void*)"analyzeMeasurands", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { "&Visibility graph", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "Neighborhood size", 0, (Fl_Callback*)cbMenu, (void*)"analyzeNeighbors", 0, FL_NORMAL_LABEL,0,textSize },
        { "2nd order neighborhood", 0, (Fl_Callback*)cbMenu, (void*)"analyzeNeighbors2", 0, FL_NORMAL_LABEL,0,textSize },
        { "Clustering coefficient", 0, (Fl_Callback*)cbMenu, (void*)"analyzeClusterField", 0, FL_NORMAL_LABEL,0,textSize },
        { "Revelation by 1 step", 0, (Fl_Callback*)cbMenu, (void*)"analyzeRevelField1", 0, FL_NORMAL_LABEL,0,textSize },
        { "Isovist perimeter", 0, (Fl_Callback*)cbMenu, (void*)"analyzePeriLength", 0, FL_NORMAL_LABEL,0,textSize },
        { "Isovist perimeter nVertices", 0, (Fl_Callback*)cbMenu, (void*)"analyzeNVertices", 0, FL_NORMAL_LABEL,0,textSize },
        { "Isovist perimeter² / &area", 0, (Fl_Callback*)cbMenu, (void*)"analyzePeriArea", 0, FL_NORMAL_LABEL,0,textSize },
        { "Perimeter openness ratio", 0,(Fl_Callback*)cbMenu, (void*)"analyzeOpenness",  0, FL_NORMAL_LABEL,0,textSize },
        { "Free near space ratio",    0,(Fl_Callback*)cbMenu, (void*)"analyzeVisRatio2", 0, FL_NORMAL_LABEL,0,textSize },
        { "Free medium space ratio",  0,(Fl_Callback*)cbMenu, (void*)"analyzeVisRatio4", 0, FL_NORMAL_LABEL,0,textSize },
        { "Minimum wall distance",  0,(Fl_Callback*)cbMenu, (void*)"analyzeMinDist", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { "&Meta Analyis", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "Minima", 0, (Fl_Callback*)cbMenu, (void*)"metaMinima", 0, FL_NORMAL_LABEL,0,textSize },
        { "Maxima", 0, (Fl_Callback*)cbMenu, (void*)"metaMaxima", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { "&Partial Isovist", 0, (Fl_Callback*)cbMenu, (void*)"analyzePartialIsovist", 0, FL_NORMAL_LABEL,0,textSize },
        { "&Help", 0,0,0, FL_SUBMENU, FL_NORMAL_LABEL,0,textSize },
        { "&Help...", 0, (Fl_Callback*)cbMenu, (void*)"helpHelp", 0, FL_NORMAL_LABEL,0,textSize },
        { "&About", 0, (Fl_Callback*)cbMenu, (void*)"helpAbout", 0, FL_NORMAL_LABEL,0,textSize },
        { 0 },
        { 0 }
    };
    pDevWindow->menu(menuItem);

    geoFileHandler::addLoader(ioObj::load,"obj");
    pDevGraphics = new deviceGraphicsFltk(0,20,pDevWindow->w(),pDevWindow->h()-40,ini);
    pDevWindow->visual(pDevGraphics);
    pDevGraphics->initGraphics();
    pDevGraphics->take_focus();

#ifndef WIN32
    pHelpDialog= new Fl_Help_Dialog;
#endif

    // visibility graph initialization:
    pVgr=new visiGraph(ini);
    pDevGraphics->addNode(pVgr);

    // vars for camera control / input handling:
    bool button2Pressed=false;
    float mouseDownX=0;
    float mouseDownY=0;
    unsigned int inputState=0;
    vec6f inputAxis;

    // timing variables:
    const double stdUpdate=0.1;
    double updateSpeed=stdUpdate;
    double tNow=chrono::stamp();
    double deltaT=0;
    double tLastEvt=0;
    double deltaEvt=0.5;

    // main loop:
    while(!(inputState&BIT[EVT_TERM])&&pDevWindow->shown()) {
        deltaT=chrono::stamp()-tNow;
        tNow+=deltaT;

        pDevGraphics->getInput(inputAxis,inputState);

        switch(pVgr->state()) { // handle interactive adjustments:
        case VGR_STATE_POSITION:
            if(pGraphSettings) {
                if(pDevWindow->buttonOk()) {
                    pVgr->nextStep();
                    pDevWindow->showOkCancel();
                    delete pGraphSettings;
                    pGraphSettings=0;
                }
                if(pDevWindow->stop()) {
                    pVgr->clear();
                    pDevWindow->showInfoR();
                    pDevGraphics->dropNode(pPermeability);
                    delete pPermeability;
                    pPermeability=0;
                    delete pGraphSettings;
                    pGraphSettings=0;
                }
            }
            break;
        case VGR_STATE_ADDREMOVE:
            if((inputState & BIT[BUTTON_0])&&(tNow>=tLastEvt+deltaEvt)) {
                pVgr->switchCell(pDevGraphics->mouseRay());
                tLastEvt=tNow;
            }
            if(pDevWindow->buttonOk()) {
                char * fileName=fl_file_chooser("Select boundary model","*.{wrl,WRL,3ds,3DS,x3d,X3D,obj,OBJ}",NULL);
                if(!fileName) {
                    pDevWindow->infoL("A boundary (wall) model has not been selected.");
                    break;
                }
                pDevGraphics->dropNode(pPermeability);
                delete pPermeability;
                pPermeability=0;
                pBoundary=geoFileHandler::load(fileName);
                pVgr->boundaryModel(*pBoundary,fileName);
                pDevGraphics->addNode(pBoundary);
                pVgr->nextStep();
            }
            break;
        case VGR_STATE_COMPUTE:
            updateSpeed=0.0f;
            if(pDevWindow->stop()) {
                pVgr->clear();
                pDevWindow->infoL("computing graph aborted.");
                pDevWindow->showInfoR();
            }
            else for(unsigned int i=0; i<4; ++i) // factor 4 for being faster
                if(pVgr->state()==VGR_STATE_COMPUTE)
					pVgr->nextStep();
            break;
        case VGR_STATE_COMPUTATION_DONE:
            updateSpeed=stdUpdate;
            pDevWindow->showInfoR();
            pVgr->nextStep();
            break;
        case VGR_STATE_CLUSTERING:
        case VGR_STATE_NBH2:
        case VGR_STATE_NVTX: {
            updateSpeed=0.0f;
			unsigned int currState=pVgr->state();
            if(pDevWindow->stop()) {
                pVgr->state(VGR_STATE_COMPUTATION_DONE);
                pDevWindow->infoL("computation aborted.");
                updateSpeed=stdUpdate;
            }
            else for(unsigned int i=0; i<4; ++i) // factor 4 for being faster
                if(pVgr->state()==currState) pVgr->nextStep();
            break;
		}
        case VGR_STATE_ISOVIST1:
            if(inputState & BIT[BUTTON_0]) pVgr->showIsovist(pDevGraphics->mouseRay(), pVgr->angleStart(), pVgr->angleEnd());
            break;
        case VGR_STATE_ISOVIST2:
            if(inputState & BIT[BUTTON_0]) pVgr->showIsovist2(pDevGraphics->mouseRay());
            break;
        case VGR_STATE_REVELATION1:
            if(inputState & BIT[BUTTON_0]) pVgr->showRevelation1(pDevGraphics->mouseRay());
            break;
        case VGR_STATE_CLUSTERING1:
            if(inputState & BIT[BUTTON_0]) pVgr->showClustering(pDevGraphics->mouseRay());
            break;
        case VGR_STATE_PERIMETER:
            if(inputState & BIT[BUTTON_0]) pVgr->showPerimeter(pDevGraphics->mouseRay(), pVgr->angleStart(), pVgr->angleEnd());
            break;
        case VGR_STATE_MEASURANDS:
            if((inputState & BIT[BUTTON_0])&&(tNow>=tLastEvt+deltaEvt)) {
                string s=pVgr->measurands(pDevGraphics->mouseRay());
                if(pConsole->size()) s=s.substr(s.find("\n")+1);
                (*pConsole) << s;
                tLastEvt=tNow;
            }
            break;
        case VGR_STATE_PCA:
            if(inputState & BIT[BUTTON_0]) pVgr->showPCA(pDevGraphics->mouseRay());
            break;
        case VGR_STATE_CIRCUM2:
            if(inputState & BIT[BUTTON_0]) pVgr->showCircum(pDevGraphics->mouseRay(),2.0f);
            break;
        case VGR_STATE_CIRCUM4:
            if(inputState & BIT[BUTTON_0]) pVgr->showCircum(pDevGraphics->mouseRay(),4.0f);
            break;
        case VGR_STATE_PROFILE:
            if((inputState & BIT[BUTTON_0])&&(tNow>=tLastEvt+deltaEvt)) {
                (*pConsole) << pVgr->profile(pDevGraphics->mouseRay(),1.0f, pVgr->angleStart(), pVgr->angleEnd());
                tLastEvt=tNow;
            }            
            break;
        case VGR_STATE_PARTIAL:
            if((inputState & BIT[BUTTON_0])&&(tNow>=tLastEvt+deltaEvt)) {
                string s= pVgr->partial(pDevGraphics->mouseRay(),1.0f, pVgr->angleStart(), pVgr->angleEnd());
                if(pConsole->size()) s=s.substr(s.find("\n")+1);
                (*pConsole) << s;
                tLastEvt=tNow;
            }            
            break;
        default:
            if(inputState & BIT[BUTTON_0]) {
                float currValue=pVgr->value(pDevGraphics->mouseRay());
                pDevWindow->infoR(f2s(currValue)+" ("+f2s(pVgr->lastIntersection()[X],2)+','+f2s(pVgr->lastIntersection()[Y],2)+")");
            }
        }

        // interpret text input:
        string inputText=pDevWindow->inputText();
        if(inputText.size()) {
            vector<string> vStr;
            if(split(inputText,vStr,",")>1) {
                float coordX=s2f(vStr[0]);
                float coordY=s2f(vStr[1]);
                float currValue=pVgr->value(coordX,coordY);
                pDevWindow->infoR(f2s(currValue)+" ("+f2s(pVgr->lastIntersection()[X],2)+','+f2s(pVgr->lastIntersection()[Y],2)+")");
                if(pVgr->state()==VGR_STATE_MEASURANDS) {
                    line ray(coordX,coordY,100.0f,coordX,coordY,-100.0f);
                    string s=pVgr->measurands(ray);
                    if(pConsole->size()) s=s.substr(s.find("\n")+1);
                    (*pConsole) << s;
                }
                else if(pVgr->state()==VGR_STATE_PROFILE) {
                    line ray(coordX,coordY,100.0f,coordX,coordY,-100.0f);
                    (*pConsole) << pVgr->profile(ray,1.0f, pVgr->angleStart(), pVgr->angleEnd());
                }
                else if(pVgr->state()==VGR_STATE_PARTIAL) {
                    line ray(coordX,coordY,100.0f,coordX,coordY,-100.0f);
                    string s= pVgr->partial(ray,1.0f, pVgr->angleStart(), pVgr->angleEnd());
                    if(pConsole->size()) s=s.substr(s.find("\n")+1);
                    (*pConsole) << s;
                }
            }
            else pDevWindow->infoR("ERROR:["+inputText+']');
        }

        if(pVgr->progress()>=0.0f) {
            pDevWindow->showProgress();
            pDevWindow->progress(pVgr->progress());
        }

        // camera control:
        vec6f & pos=pDevGraphics->camera();
        if(inputState & BIT[BUTTON_2]) {
            if(!button2Pressed) {
                fl_cursor(FL_CURSOR_MOVE);
                updateSpeed=0;
                button2Pressed=true;
            }
            else {
                pos[X]+=(inputAxis[H]-mouseDownX)*pos[Z]*0.5f;
                pos[Y]-=(inputAxis[P]-mouseDownY)*pos[Z]*0.5f;
            }
            mouseDownX=inputAxis[H];
            mouseDownY=inputAxis[P];
        }
        else if(button2Pressed) {
            fl_cursor(FL_CURSOR_DEFAULT);
            button2Pressed=false;
            updateSpeed=stdUpdate;
        }
        if(inputAxis[X]||inputAxis[Y]||inputAxis[Z]) {
            if(updateSpeed) updateSpeed=0.0f;
            else {
                const float camTranslSpeed=1.0f;
                pos[X]-=inputAxis[X]*deltaT*camTranslSpeed*pos[Z]*0.5f;
                pos[Y]-=inputAxis[Y]*deltaT*camTranslSpeed*pos[Z]*0.5f;
                pos[Z]+=inputAxis[Z]*deltaT*camTranslSpeed*5.0f;
            }
        }
        else updateSpeed=stdUpdate;
        pos[Z]+=inputAxis[R]*2;

        string s(pVgr->message());
        if(s.size()) pDevWindow->infoL(s);
        pDevGraphics->redraw();
        Fl::wait(updateSpeed); // update fltk windows
    }
    return 0;
}
