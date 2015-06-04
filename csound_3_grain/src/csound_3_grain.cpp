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
#include "CinderOpenCv.h"

#include "csound.hpp"
#include "csound.h"

#include "ContourMap.h"
#include "ufUtil.h"
#include "ConsoleColor.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    
    Csound * csound;
    MYFLT mAmp;
    MYFLT mFreq;
    
    Perlin mPln;
    Perlin mPln2;

};

void cApp::setup(){
    
    mAmp = 0.5;
    mFreq = 200;
    mPln.setSeed(123);
    mPln.setOctaves(4);
    mPln2.setSeed(555);
    mPln2.setOctaves(32);

    csound = new Csound();
    //csound->SetOption( (char*)"-odac" );
    
    string fileName = "-o" + uf::getTimeStamp() + ".wav";
    csound->SetOption( const_cast<char*>(fileName.c_str()) );   // file name
    csound->SetOption("-W");                // Wav
    csound->SetOption("-f");                // 32float
    
    std::string orc =   R"dlm(

        ; orchestra code

        sr=192000
        ksmps=129
        nchnls=2
        0dbfs=1

        giCosine	ftgen	0, 0, 8193, 9, 1, 1, 90		; cosine
        giDisttab	ftgen	0, 0, 32768, 7, 0, 32768, 1	; for kdistribution
        giFile		ftgen	0, 0, 0, 1, "0524_1623_34.wav", 0, 0, 0	; soundfile for source waveform
        giWin		ftgen	0, 0, 4096, 20, 9, 1		; grain envelope
        giPan		ftgen	0, 0, 32768, -21, 1		; for panning (random values between 0 and 1)
            
            
        ; *************************************************
        ; partikkel example, processing of soundfile
        ; uses the file "fox.wav"
        ; *************************************************
        instr 1
        
        /*score parameters*/
        ispeed			= p4		; 1 = original speed
        igrainrate		= p5		; grain rate
        igrainsize		= p6		; grain size in ms
        icent			= p7		; transposition in cent
        iposrand		= p8		; time position randomness (offset) of the pointer in ms
        icentrand		= p9		; transposition randomness in cents
        ipan			= p10		; panning narrow (0) to wide (1)
        idist			= p11		; grain distribution (0=periodic, 1=scattered)
        
        /*get length of source wave file, needed for both transposition and time pointer*/
        ifilen			tableng	giFile
        ifildur			= ifilen / sr
        
        /*sync input (disabled)*/
        async			= 0
        
        /*grain envelope*/
        kenv2amt		= 1		; use only secondary envelope
        ienv2tab 		= giWin		; grain (secondary) envelope
        ienv_attack		= -1 		; default attack envelope (flat)
        ienv_decay		= -1 		; default decay envelope (flat)
        ksustain_amount		= 0.5		; no meaning in this case (use only secondary envelope, ienv2tab)
        ka_d_ratio		= 0.5 		; no meaning in this case (use only secondary envelope, ienv2tab)
        
        /*amplitude*/
        kamp			= 0.4*0dbfs	; grain amplitude
        igainmasks		= -1		; (default) no gain masking
        
        /*transposition*/
        kcentrand		rand icentrand	; random transposition
        iorig			= 1 / ifildur	; original pitch
        kwavfreq		= iorig * cent(icent + kcentrand)
        
        /*other pitch related (disabled)*/
        ksweepshape		= 0		; no frequency sweep
        iwavfreqstarttab 	= -1		; default frequency sweep start
        iwavfreqendtab		= -1		; default frequency sweep end
        awavfm			= 0		; no FM input
        ifmamptab		= -1		; default FM scaling (=1)
        kfmenv			= -1		; default FM envelope (flat)
        
        /*trainlet related (disabled)*/
        icosine			= giCosine	; cosine ftable
        kTrainCps		= igrainrate	; set trainlet cps equal to grain rate for single-cycle trainlet in each grain
        knumpartials		= 1		; number of partials in trainlet
        kchroma			= 1		; balance of partials in trainlet
        
        /*panning, using channel masks*/
        imid			= .5; center
        ileftmost		= imid - ipan/2
        irightmost		= imid + ipan/2
        giPanthis		ftgen	0, 0, 32768, -24, giPan, ileftmost, irightmost	; rescales giPan according to ipan
        tableiw		0, 0, giPanthis				; change index 0 ...
        tableiw		32766, 1, giPanthis			; ... and 1 for ichannelmasks
        ichannelmasks		= giPanthis		; ftable for panning
                
        /*random gain masking (disabled)*/
        krandommask		= 0
                
        /*source waveforms*/
        kwaveform1		= giFile	; source waveform
        kwaveform2		= giFile	; all 4 sources are the same
        kwaveform3		= giFile
        kwaveform4		= giFile
        iwaveamptab		= -1		; (default) equal mix of source waveforms and no amplitude for trainlets
            
        /*time pointer*/
        afilposphas		phasor ispeed / ifildur
        /*generate random deviation of the time pointer*/
        iposrandsec		= iposrand / 1000	; ms -> sec
        iposrand		= iposrandsec / ifildur	; phase values (0-1)
        krndpos			linrand	 iposrand	; random offset in phase values
        /*add random deviation to the time pointer*/
        asamplepos1		= afilposphas + krndpos; resulting phase values (0-1)
        asamplepos2		= asamplepos1
        asamplepos3		= asamplepos1
        asamplepos4		= asamplepos1
        
        /*original key for each source waveform*/
        kwavekey1		= 1
        kwavekey2		= kwavekey1
        kwavekey3		= kwavekey1
        kwavekey4		= kwavekey1
        
        /* maximum number of grains per k-period*/
        imax_grains		= 100
        
        aL, aR		partikkel igrainrate, idist, giDisttab, async, kenv2amt, ienv2tab, \
        ienv_attack, ienv_decay, ksustain_amount, ka_d_ratio, igrainsize, kamp, igainmasks, \
        kwavfreq, ksweepshape, iwavfreqstarttab, iwavfreqendtab, awavfm, \
        ifmamptab, kfmenv, icosine, kTrainCps, knumpartials, \
        kchroma, ichannelmasks, krandommask, kwaveform1, kwaveform2, kwaveform3, kwaveform4, \
        iwaveamptab, asamplepos1, asamplepos2, asamplepos3, asamplepos4, \
        kwavekey1, kwavekey2, kwavekey3, kwavekey4, imax_grains
        
        outs			aL, aR
        
        endin
    )dlm";

    {
        cout << orc << endl;
        int result = csound->CompileOrc( orc.c_str() );
        
        if( result ==0 ){
            ccout::b( "Orcestra file compile OK" );
        }else{
            ccout::r( "Orcestra file compile Failed" );
            quit();
        }
    }
    
    
    std::string sco =   R"dlm(
    
        ; score code
        ;i1	st	dur	speed	grate	gsize	cent	posrnd	cntrnd	pan	dist
        i1	0	2.757	1	200	15	0	0	0	0	0
        s
        i1	0	2.757	1	200	15	400	0	0	0	0
        s
        i1	0	2.757	1	15	450	400	0	0	0	0
        s
        i1	0	2.757	1	15	450	400	0	0	0	0.4
        s
        i1	0	2.757	1	200	15	0	400	0	0	1
        s
        i1	0	5.514	.5	200	20	0	0	600	.5	1
        s
        i1	0	11.028	.25	200	15	0	1000	400	1	1
    )dlm";
    
    {
        cout << sco << endl;
        int result = csound->ReadScore(sco.c_str());
        if( result ==0 ){
            ccout::b("Score file compile OK");
        }else{
            ccout::r("Score file compile Failed");
        }
    }
    
    csound->ReadScore(sco.c_str());
    csound->Start();
    
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
    }
    
    csoundDestroy( csound->GetCsound() );
    quit();
}

void cApp::update(){
}

void cApp::draw(){
    gl::clear();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
