//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Perlin.h"
#include "cinder/params/Params.h"

#include "mtUtil.h"
#include "ConsoleColor.h"
#include "Exporter.h"
#include "Ramses.h"


using namespace ci;
using namespace ci::app;
using namespace std;


class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    
    MayaCamUI camUi;
    Perlin mPln;
    Exporter mExp;
    
    vector<Ramses> rms;
    bool bStart = false;
    bool bOrtho = false;
    int eSimType = 0;
    int frame = 100;

    params::InterfaceGlRef gui;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    
    //float w = 1080*3;
    //float h = 1920;

    float w = 1920;
    float h = 1080;
    
    setWindowSize( w, h );
    mExp.setup( w, h, 0, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam(w, h, 60.0f, 0.1, 1000000 );
    cam.lookAt( Vec3f(0,0,650), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    gui = params::InterfaceGl::create( getWindow(), Ramses::simType[eSimType], Vec2i(300, getWindowHeight()) );
    gui->setOptions( "", "position=`0 0` valueswidth=100" );

    for( int i=0; i<6; i++){
        Ramses r(eSimType,i);
        rms.push_back( r );
    }
    
    function<void(void)> update = [&](){
        for( int i=0; i<rms.size(); i++){ rms[i].updateVbo(); }
    };
    
    gui->addText( "main" );
    gui->addParam("start", &bStart );
    gui->addParam("frame", &frame ).updateFn(update);
    gui->addParam("ortho", &bOrtho );
    gui->addParam("xyz global scale", &Ramses::globalScale ).step(0.01).updateFn(update);
    gui->addParam("r(x) resolution", &Ramses::boxelx, true );
    gui->addParam("theta(y) resolution", &Ramses::boxely, true );

    gui->addSeparator();
    
    for( int i=0; i<6; i++){
        string p = Ramses::prm[i];
        //gui->addText( p );
        
        //function<void(void)> up = bind(&Ramses::updateVbo, &rms[i]);
        function<void(void)> up = [i, this](){
            rms[i].updateVbo();
        };
        
        function<void(void)> up2 = [i, this](){
            rms[i].loadSimData(this->frame);
            rms[i].updateVbo();
        };
        
        gui->addParam(p+" show", &rms[i].bShow ).group(p).updateFn(up2);
        gui->addParam(p+" Auto Min Max", &rms[i].bAutoMinMax ).group(p).updateFn(up);
        gui->addParam(p+" in min", &rms[i].in_min).step(0.05f).group(p).updateFn(up);
        gui->addParam(p+" in max", &rms[i].in_max).step(0.05f).group(p).updateFn(up);

        gui->addParam(p+" z extrude", &rms[i].extrude).step(1.0f).group(p).updateFn(up);
        gui->addParam(p+" z offset", &rms[i].zoffset).step(1.0f).group(p).updateFn(up);

        gui->addParam(p+" xy scale", &rms[i].scale).step(1.0f).group(p).updateFn(up);
        //gui->addParam(p+" visible thresh", &rms[i].visible_thresh).step(0.005f).min(0.0f).max(1.0f).group(p).updateFn(up);
        gui->addParam(p+" log", &rms[i].eStretch).step(1).min(0).max(1).group(p).updateFn(up2);

        // read only
        gui->addParam(p+" visible rate(%)", &rms[i].visible_rate, true ).group(p);
        gui->addParam(p+" num particle", &rms[i].nParticle, true).group(p);

        gui->addSeparator();
    }
    
#ifdef RENDER
    mExp.startRender();
#endif
    
}


void cApp::update(){
    if( bStart ){
        for( int i=0; i<rms.size(); i++){
            if( rms[i].bShow ){
                rms[i].loadSimData( frame );
                rms[i].updateVbo();
            }
        }
    }
}

void cApp::draw(){
    
    gl::enableAlphaBlending();
    glPointSize(1);
    glLineWidth(1);
    
    bOrtho ? mExp.beginOrtho( true ) : mExp.begin( camUi.getCamera() );
    {
        
        gl::clear();    
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        if( !mExp.bRender && !mExp.bSnap ){
            mt::drawCoordinate(10);
            gl::color(1, 0, 0);
            gl::drawStrokedCircle( Vec2i(0,0), 20);
        }
        
        for( int i=0; i<rms.size(); i++){
            rms[i].draw();
        }
    }mExp.end();
    
    mExp.draw();
    
    if(gui) gui->draw();

    if( bStart)frame++;
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S': mExp.startRender();  break;
        case 'T': mExp.stopRender();  break;
        case 's': mExp.snapShot();  break;
        case ' ': bStart = !bStart; break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
