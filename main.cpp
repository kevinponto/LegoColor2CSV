// Shipwreck.cpp : Defines the entry point for the console application.
//

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

using namespace cv;
using namespace std;

Point2f src_vertices[4];
Point2f dst_vertices[4];

string gFilename = "scans/WhatColor_inPersonDataBlocks_WISciFest2018_Page_01_Image_0001.png";

int clicknum=0;

Mat img;//src image
Mat wimg; //warp image
Mat simg; //simple image

int gridX=13, gridY=21;
bool gDrawGrid=false;
bool gMakeKey = true;

float gScale = .5;

int clusterCount = 12;

void on_slider( int, void* );
void on_button( int, void* );

struct Key
{
    string name;
    Scalar color;

};

vector<Key> keys;

void loadKeys()
{
    vector<string> colors;
    colors.push_back("Green");
    colors.push_back("Light Green");
    colors.push_back("Blue");
    colors.push_back("Black");
    colors.push_back("Brown");
    colors.push_back("Pink");
    colors.push_back("Purple");
    colors.push_back("Red");
    colors.push_back("Yellow");
    colors.push_back("Grey");
    colors.push_back("White");
    colors.push_back("Orange");
    
    for (int i=0; i < colors.size(); i++)
    {
        Mat bgr_image = imread("colors/" + colors[i] + ".png");
        if (!bgr_image.empty())
        {
            Mat lab_image;
            cv::cvtColor(bgr_image, lab_image, CV_BGR2Lab);
        
            Key k;
            k.color = mean(lab_image);
            k.name = colors[i];
            
            keys.push_back(k);
            
            //debug
            printf("%s \t %f %f %f\n", k.name.c_str(), k.color[0], k.color[1], k.color[2]);
        }
    }
    
    
}

string getBestColor(Vec3b v)
{
    string name;
    float distance = -1;
    
    for (int i=0; i < keys.size(); i++)
    {
        float d = (keys[i].color[0] - v[0])*(keys[i].color[0] - v[0]) +
                    (keys[i].color[1] - v[1])*(keys[i].color[1] - v[1]) +
                    (keys[i].color[2] - v[2])*(keys[i].color[2] - v[2]);
        
        if (distance < 0 || d < distance)
        {
            
            distance = d;
            name = keys[i].name;
        }
    }
    
    return name;
}


vector<Vec3b> getGridColors(Mat src)
{
    vector<Vec3b> colors;
    
    int w = src.cols / gridX;
    int h = src.rows / gridY;
    
    for( int y = 0; y < src.rows; y+=h )
    for( int x = 0; x < src.cols; x+=w )
    {
        Vec3f c(0,0,0);
        for( int i = 0; i < h; i++ )
        for( int j = 0; j < w; j++ )
        {
            Vec3b b =src.at<Vec3b>(i,j);
            c += Vec3f(b[0], b[1], b[2]);
        }
        
        
    }
    
    return colors;
}

cv::Scalar getColorFromGrid(int x, int y, int window=5)
{
    Mat src = wimg;
    int w = src.cols / gridX;
    int h = src.rows / gridY;
    
    int i = h*y + .5*h;
    int j = w*x + .5*w;
    
    //Vec3b b =src.at<Vec3b>(i,j);
    //return cv::Scalar(b[0], b[1], b[2]);
    
    Scalar s(0,0,0);
    float weight=0;
    for (int ioff = -window; ioff <= window; ioff++)
        for (int joff = -window; joff <= window; joff++)
        {
            Vec3b b =src.at<Vec3b>(i+ioff,j+joff);
            s+=cv::Scalar(b[0], b[1], b[2]);
            weight++;
        }
    
    s/= weight;
    return s;
}

Mat MakeSimpleImage( int w=20, int h = 20)
{

    Mat timg = Mat(gridY*h, gridX *w, CV_8UC3);
    
    
    for (int x=0; x < gridX; x++)
     for (int y=0; y < gridY; y++)
    {
        cv::Rect rect(x*w, y*h, w, h);
        
        cv::rectangle(timg, rect, getColorFromGrid(x,y), CV_FILLED);
    }
    
    return timg;
}

void DrawGrid()
{
    // Draw a line
    for (int i=0; i < wimg.cols; i+= (wimg.cols / gridX))
        line( wimg, Point( i, 0 ), Point( i, wimg.rows), Scalar( 255, 255, 255 ),  2, 8 );
    
    for (int j=0; j < wimg.rows; j+= (wimg.rows / gridY))
        line( wimg, Point( 0, j ), Point( wimg.cols, j), Scalar( 255, 255, 255  ),  2, 8 );
    
    //show the image
    imshow("warp", wimg);
    
}

void WriteColorCloud()
{
    Mat src = MakeSimpleImage(1, 1);
    

    {
       
        FILE *f = fopen ("colors.xyz", "w");
        for( int y = 0; y < src.rows; y++ )
        {
            for( int x = 0; x < src.cols; x++ )
            {
                Vec3b v = src.at<Vec3b>(y,x);
                
                fprintf(f, "%d %d %d %d %d %d\n", v[0], v[1], v[2], v[0], v[1], v[2]);
            }
            
        }
        fclose(f);
    }
    
}


void WriteCSV()
{
    loadKeys();
    
    WriteColorCloud();
    
    //first make an image with 1 pixel per entry
    Mat src = MakeSimpleImage(1, 1);
    
    //lets do key matching
    {
        Mat lab_image;
        cv::cvtColor(src, lab_image, CV_BGR2Lab);
    
        FILE *f = fopen ("colors.csv", "w");
        for( int y = 0; y < lab_image.rows; y++ )
        {
            for( int x = 0; x < lab_image.cols; x++ )
            {
                Vec3b v = lab_image.at<Vec3b>(y,x);
            
                fprintf(f, "%s,", getBestColor(v).c_str());
            }
            fprintf(f, "\n");
        }
        fclose(f);
    }
    
    //lets do key matching
    {
        Mat lab_image;
        cv::cvtColor(src, lab_image, CV_BGR2Lab);
        
        FILE *f = fopen ((gFilename + ".csv").c_str(), "w");
        
        for( int x = 0; x < lab_image.cols; x++ )
        {
            for( int y = 0; y < lab_image.rows; y++ )
            {
                Vec3b v = lab_image.at<Vec3b>(y,x);
                
                fprintf(f, "%s,", getBestColor(v).c_str());
            }
            fprintf(f, "\n");
        }
        fclose(f);
    }
    
    Mat samples(src.rows * src.cols, 3, CV_32F);
    for( int y = 0; y < src.rows; y++ )
        for( int x = 0; x < src.cols; x++ )
            for( int z = 0; z < 3; z++)
                samples.at<float>(y + x*src.rows, z) = src.at<Vec3b>(y,x)[z];
    

    Mat labels;
    int attempts = 20;
    Mat centers;
    kmeans(samples, clusterCount, labels, TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.0001), attempts, KMEANS_PP_CENTERS, centers );
    
    if (gMakeKey)
    {
        Mat kimg(clusterCount, 1, CV_8UC3);
        
        
        FILE *fk = fopen("key.txt", "w");
        for (int i = 0; i < clusterCount; i++)
        {
             fprintf(fk, "%f, %f, %f,\n",
                     centers.at<float>(i, 0),
                     centers.at<float>(i, 1),
                     centers.at<float>(i, 2)
                     );
            kimg.at<Vec3b>(i,0)[0] = centers.at<float>(i, 0);
            kimg.at<Vec3b>(i,0)[1] = centers.at<float>(i, 1);
            kimg.at<Vec3b>(i,0)[2] = centers.at<float>(i, 2);
            
            Mat oimg(20, 20, CV_8UC3);
            oimg.setTo(cv::Scalar(centers.at<float>(i, 0),centers.at<float>(i, 1),centers.at<float>(i, 2)));
            char fname[1024];
            sprintf(fname, "colors/%d.png", i);
            imwrite(fname, oimg);
            
            
        }
        fclose(fk);
        imwrite("key.png", kimg);
    }
    
    FILE *f = fopen ("test.csv", "w");
    for( int y = 0; y < src.rows; y++ )
    {
        for( int x = 0; x < src.cols; x++ )
        {
            int cluster_idx = labels.at<int>(y + x*src.rows,0);
            fprintf(f, "%d,", cluster_idx);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    
    
    imwrite(gFilename + "-s.png", src);
                
}

Mat runKmeans(Mat src)
{

    
    Mat samples(src.rows * src.cols, 3, CV_32F);
    for( int y = 0; y < src.rows; y++ )
    for( int x = 0; x < src.cols; x++ )
    for( int z = 0; z < 3; z++)
    samples.at<float>(y + x*src.rows, z) = src.at<Vec3b>(y,x)[z];
    
    
    Mat labels;
    int attempts = 20;
    Mat centers;
    kmeans(samples, clusterCount, labels, TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.0001), attempts, KMEANS_PP_CENTERS, centers );
    
    
    Mat new_image( src.size(), src.type() );
    for( int y = 0; y < src.rows; y++ )
    for( int x = 0; x < src.cols; x++ )
    {
        int cluster_idx = labels.at<int>(y + x*src.rows,0);
        new_image.at<Vec3b>(y,x)[0] = centers.at<float>(cluster_idx, 0);
        new_image.at<Vec3b>(y,x)[1] = centers.at<float>(cluster_idx, 1);
        new_image.at<Vec3b>(y,x)[2] = centers.at<float>(cluster_idx, 2);
    }
    return  new_image;
}

void Cluster()
{
    Mat kimg =runKmeans(simg);
    
    //Create a window
    namedWindow("k", 1);
    
    //show the image
    imshow("k", kimg);
}

void optimizeCorners()
{
    int win_size = 15;
    
    Mat gimage;
    cv::cvtColor(img, gimage, CV_BGR2GRAY);
    
    std::vector<cv::Point2f> cornersA;
    for (int i= 0; i < 4; i++)
        cornersA.push_back(src_vertices[i]);
    
    cv::cornerSubPix( gimage, cornersA, Size( win_size, win_size ), Size( -1, -1 ),
                     TermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03 ) );
    
    for (int i= 0; i < 4; i++)
        src_vertices[i] = cornersA[i];
    
}

void UpdateWindow()
{
    Mat boximg = img.clone();
    
    
    if (clicknum > 1)
        line(boximg,src_vertices[0], src_vertices[1], Scalar(0,255,0), 8);
    if (clicknum > 2)
        line(boximg,src_vertices[1], src_vertices[2], Scalar(0,255,0), 8);
    if (clicknum > 3)
        line(boximg,src_vertices[2], src_vertices[3], Scalar(0,255,0), 8);
    if (clicknum > 3)
        line(boximg,src_vertices[3], src_vertices[0], Scalar(0,255,0), 8);
    

    //show the image
    imshow("Photograph", boximg);

}



void fixWarp()
{
   
    
    Mat warpMatrix = getPerspectiveTransform(src_vertices, dst_vertices);
    warpPerspective(img, wimg, warpMatrix, wimg.size(), INTER_LINEAR, BORDER_CONSTANT);
    
    if (gDrawGrid)
        DrawGrid();

    //Create a window
    namedWindow("warp", 1);
    
    //create sliders?
    createTrackbar( "Grid Size X","warp", &gridX, 50, on_slider );
    createTrackbar( "Grid Size Y","warp", &gridY, 50, on_slider );
   // createButton("Toggle Grid", on_button, NULL, QT_PUSH_BUTTON, 0);
   // createButton("button2",on_button,NULL,CV_CHECKBOX,0);
    
    //show the image
    imshow("warp", wimg);
    
}
                 
void on_button(int state, void* userdata)
 {
     
 }

void on_slider( int, void* )
{
    fixWarp();
    /*
     alpha = (double) alpha_slider/alpha_slider_max ;
     beta = ( 1.0 - alpha );
     
     addWeighted( src1, alpha, src2, beta, 0.0, dst);
     
     imshow( "Linear Blend", dst );
     */
}

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if  ( event == EVENT_LBUTTONDOWN )
    {
     //   cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        printf("click at %d %d \n", x, y);
        
        src_vertices[clicknum%4] = Point(x,y);
        
        clicknum++;
        if (clicknum >= 4)
            fixWarp();
        
        UpdateWindow();
    }
    else if  ( event == EVENT_RBUTTONDOWN )
    {
     //   cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
    }
    else if  ( event == EVENT_MBUTTONDOWN )
    {
     //   cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
    }
    else if ( event == EVENT_MOUSEMOVE )
    {
     //   cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;
        
    }
}


#ifdef WIN_32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  
    if (argc > 1)
    {
        gFilename = argv[1];
        printf("set filename to %s\n", gFilename.c_str());
    }
    

    
    Mat oimg = imread(gFilename);
    
    Mat gimg = oimg.clone();
    /*
    imwrite(gFilename + "-g.png", gimg);
    printf("what is going on?\n");
    
    return 10;
    */
    
    gScale =  800 / float(oimg.cols);
    
    
    Size size(oimg.cols*gScale,oimg.rows*gScale);//the dst image size,e.g.100x100
    
   
    resize(oimg,img,size);//resize image
    int blocksize=60;
    wimg = Mat(gridX*blocksize, gridY *blocksize*.7, CV_8UC3);//img.clone();//Mat (200,400, CV_8U);
    //now make the dst rects
    dst_vertices[0] = Point(0, 0);
    dst_vertices[1] = Point(wimg.cols, 0);
    dst_vertices[2] = Point(wimg.cols, wimg.rows);
    dst_vertices[3] = Point(0, wimg.rows);
    
    
    //Create a window
    namedWindow("Photograph", CV_WINDOW_AUTOSIZE);
    
    //set the callback function for any mouse event
    setMouseCallback("Photograph", CallBackFunc, NULL);
    
    //show the image
    //imshow("My Window", img);
    UpdateWindow();
    
    // Wait until user press some key
    int key = waitKey(0);
    
    int targetNum=0;
    
    while (key != 27)
    {
        
        key = waitKey(0);
        
        if (key >= '0' && key <= '9')
        {
            targetNum = key -'0';
            clicknum = targetNum;
            printf("%d\n", targetNum);
        }
        
        if (key == 'q')
        {
            targetNum = (targetNum-1)%4;
            printf("%d\n", targetNum);
        }
        if (key == 'e')
        {
            targetNum = (targetNum+1)%4;
            printf("%d\n", targetNum    );
        }
        
        if (key == 'g')
        {
            gDrawGrid = !gDrawGrid;
            fixWarp();
        }
        
        if (key == 'k')
            Cluster();
        
        if (key == 'i')
        {
            simg = MakeSimpleImage();
            
            //Create a window
            namedWindow("s", 1);
            
            //show the image
            imshow("s", simg);
        }
        
        int x=0, y=0;
        if (key == 'w') y++; // up
        if (key == 's') y--;   // down
        if (key == 'a') x--;   //  left
        if (key == 'd')  x++;  // right
        
        if (x || y)
        {
            
            src_vertices[targetNum%4] += Point2f(x,y);
            UpdateWindow();
            fixWarp();
        }
        
        if (key == 'o')
        {
            
            optimizeCorners();
            fixWarp();
        }
         if (key == 'p')
        {
            WriteCSV();
        }
    }
    
    return 0;
}
