#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <string>
#include <chrono>
#include <thread>

#include "Lepton3.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "stopwatch.hpp"

using namespace std;

// ----> Global variables
Lepton3* lepton3=nullptr;
static bool close = false;

cv::Mat frame16; // 16bit Lepton frame RAW frame

uint16_t min_raw16;
uint16_t max_raw16;

std::string temp_str;
std::string win_name = "Jetson Nano Fever Control - Walter Lucetti";
int raw_cursor_x = -1;
int raw_cursor_y = -1;

// Hypothesis: sensor is linear in 14bit dynamic
// If the range of the sensor is [0,150] °C in High Gain mode
double temp_scale_factor = 0.0092; // 150/(2^14-1))
double simul_temp = 0.0;
// <---- Global variables

// ----> Defines
#define FONT_FACE cv::FONT_HERSHEY_SIMPLEX

// ----> Constants
const uint16_t H_TXT_INFO = 70;
const double IMG_RESIZE_FACT = 4.0;

const cv::Scalar COL_NORM_TEMP(15,200,15);
const cv::Scalar COL_TXT(241,240,236);

const std::string STR_FEVER = "FEVER ALERT";
const std::string STR_WARN = "FEVER WARNING";
const std::string STR_NORM = "NO FEVER";
const std::string STR_NO_HUMAN = "---";

const double TEMP_MIN_HUMAN = 34.5f;
const double TEMP_WARN = 37.0f;
const double TEMP_FEVER = 37.5f;
const double TEMP_MAX_HUMAN = 42.0f;

const double fever_thresh = 0.05;
// <---- Constants

// ----> Global functions
void close_handler(int s);
void keyboard_handler(int key);

void set_rgb_mode(bool enable);
void normalizeFrame(cv::Mat& in_frame16, cv::Mat& out_frame8, double min_temp, double max_temp , double rescale_fact);
void mouseCallBackFunc(int event, int x, int y, int flags, void* userdata);

cv::Mat& analizeFrame(const  cv::Mat& frame16);
// <---- Global functions

int main (int argc, char *argv[])
{
    cout << "Check Fever App for Lepton3 on Nvidia Jetson" << std::endl;

    // ----> Set Ctrl+C handler
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = close_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    // <---- Set Ctrl+C handler

    // ----> Setup GUI
    int baseline;
    cv::Size size_norm = cv::getTextSize( STR_NORM, FONT_FACE, 0.95, 2, &baseline );
    cv::Size size_none = cv::getTextSize( STR_NO_HUMAN, FONT_FACE, 0.95, 2, &baseline );
    cv::Size size_status_str = cv::getTextSize( "STATUS", FONT_FACE, 0.75, 1, &baseline );
    // <---- Setup GUI

    Lepton3::DebugLvl deb_lvl = Lepton3::DBG_NONE;

    lepton3 = new Lepton3( "/dev/spidev0.0", "/dev/i2c-1", deb_lvl ); // use SPI1 and I2C-1 ports
    lepton3->start();

    // Disable RRGB mode to get raw 14bit values
    set_rgb_mode(false); // 16 bit raw data required

    // ----> High gain mode [0,150] Celsius
    if( lepton3->setGainMode( LEP_SYS_GAIN_MODE_HIGH ) == LEP_OK )
    {
        LEP_SYS_GAIN_MODE_E gainMode;
        if( lepton3->getGainMode( gainMode ) == LEP_OK )
        {
            string str = (gainMode==LEP_SYS_GAIN_MODE_HIGH)?string("High"):((gainMode==LEP_SYS_GAIN_MODE_LOW)?string("Low"):string("Auto"));
            cout << " * Gain mode: " << str << std::endl;
        }
    }
    // <---- High gain mode [0,150] Celsius

    // ----> Enable Radiometry to get values independent from camera temperature
    if( lepton3->enableRadiometry( true ) != LEP_OK)
    {
        cerr << "Failed to enable radiometry!" << std::endl;
        return EXIT_FAILURE;
    }
    // <---- Enable Radiometry to get values independent from camera temperature

    uint64_t frameIdx=0;

    uint8_t w,h;

    // ----> Set OpenCV output window and mouse callback
    //Create a window
    cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

    //set the callback function for any mouse event
    cv::setMouseCallback(win_name, mouseCallBackFunc, NULL);
    // <---- Set OpenCV output window and mouse callback

    StopWatch stpWtc;

    stpWtc.tic();

    bool doFFC = true;
    bool initialized = false;
    cv::Mat rgbThermFrame;
    cv::Mat textInfoFrame;
    cv::Mat displayImg;
    cv::Mat thermFrame;

    while(!close)
    {
        // Wait for the next frame (Lepton operates at 27Hz -> ~37ms per frame)
    // std::this_thread::sleep_for(std::chrono::milliseconds(37));
    
    // Retrieve last available frame
    const uint16_t* data16 = lepton3->getLastFrame16( w, h, &min_raw16, &max_raw16 );

    if(data16 == nullptr) {
        // cerr << "Error: Frame data is null." << endl;
        continue; // Skip this iteration if data is null
    }

    // Initialize frame16 if necessary
    if(!initialized || h != frame16.rows || w != frame16.cols)
    {
        frame16 = cv::Mat( h, w, CV_16UC1 );
        rgbThermFrame = cv::Mat( h*IMG_RESIZE_FACT+H_TXT_INFO, w*IMG_RESIZE_FACT, CV_8UC3, cv::Scalar(0,0,0));
        initialized = true;
    }

    // Data copy on OpenCV image
    memcpy( frame16.data, data16, w*h*sizeof(uint16_t) );

    // Normalize for displaying
    normalizeFrame( frame16, thermFrame, TEMP_MIN_HUMAN-7.5, TEMP_FEVER+1.0, temp_scale_factor );
    cv::cvtColor( thermFrame, thermFrame, cv::COLOR_GRAY2BGR );

    // Hardcoded average human temperature
    double avg_human_temp = 37.0;

    // Resize image for display
    cv::resize( thermFrame, rgbThermFrame, cv::Size(), IMG_RESIZE_FACT, IMG_RESIZE_FACT, cv::INTER_CUBIC);

    // Overlay temperature on image
    std::stringstream temp_stream;
    temp_stream << "Average Human Temp: " << avg_human_temp << " C";
    cv::putText( rgbThermFrame, temp_stream.str(), cv::Point(30, 30), FONT_FACE, 0.75, cv::Scalar(0,255,0), 2);

    // Display result
    cv::imshow( win_name, rgbThermFrame );

    // Handle window close
    int key = cv::waitKey(5);
    if( key == 'q' || key == 'Q') close=true;
}


    delete lepton3;

    return EXIT_SUCCESS;
}

void close_handler(int s)
{
    if(s==2)
    {
        cout << std::endl << "Ctrl+C pressed..." << std::endl;
        close = true;
    }
}

void set_rgb_mode(bool enable)
{
    bool rgb_mode = enable;

    if( lepton3->enableRadiometry( !rgb_mode ) < 0)
    {
        cerr << "Failed to set radiometry status" << std::endl;
    }
    else
    {
        if(!rgb_mode)
        {
            cout << " * Radiometry enabled " << std::endl;
        }
        else
        {
            cout << " * Radiometry disabled " << std::endl;
        }
    }

    // NOTE: if radiometry is enabled is unuseful to keep AGC enabled
    //       (see "FLIR LEPTON 3® Long Wave Infrared (LWIR) Datasheet" for more info)

    if( lepton3->enableAgc( rgb_mode ) < 0)
    {
        cerr << "Failed to set radiometry status" << std::endl;
    }
    else
    {
        if(!rgb_mode)
        {
            cout << " * AGC disabled " << std::endl;
        }
        else
        {
            cout << " * AGC enabled " << std::endl;
        }
    }

    if( lepton3->enableRgbOutput( rgb_mode ) < 0 )
    {
        cerr << "Failed to enable RGB output" << std::endl;
    }
    else
    {
        if(rgb_mode)
        {
            cout << " * RGB enabled " << std::endl;
        }
        else
        {
            cout << " * RGB disabled " << std::endl;
        }
    }
}

void normalizeFrame( cv::Mat& in_frame16, cv::Mat& out_frame8, double min_temp, double max_temp , double rescale_fact)
{
    // ----> Rescaling/Normalization to 8bit
    uint16_t min_scale = (uint16_t)(min_temp/rescale_fact);
    uint16_t max_scale = (uint16_t)(max_temp/rescale_fact);

    cv::Mat rescaled;
    in_frame16.copyTo(rescaled);

    double diff = static_cast<double>(max_scale - min_scale); // Image range
    double scale = 255./diff; // Scale factor

    rescaled -= min_scale; // Bias
    rescaled *= scale; // Rescale data

    rescaled.convertTo( out_frame8, CV_8U );
    // <---- Rescaling/Normalization to 8bit
}

void mouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if ( event == cv::EVENT_MOUSEMOVE )
    {
        raw_cursor_x = x/IMG_RESIZE_FACT;
        raw_cursor_y = (y-H_TXT_INFO)/IMG_RESIZE_FACT;
    }
}
