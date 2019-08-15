

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


using namespace cv;
using namespace std;

//input image
string gFilename = "Example.png";

//the images we will be using
Mat img;//src image
Mat wimg; //warp image
Mat simg; //simple image

//options for viewing where the legos are
int gGridX=13, gGridY=21;
bool gDrawGrid=true;

//number of lego colors
int gLegoColorCount = 12;

//create images for each lego color
bool gMakeKey = true;

//setting up the windows
float gScale = .5;
int gWinWidth=800;
int gWinPad=30;

//config file name
char *gConfigFilename = "config.txt";



//to determine which corner we will move
int targetNum=0;

//the corners 
Point2f src_vertices[4];
Point2f dst_vertices[4];

//prototypes
void on_slider( int, void* );
void FixWarp();
void MakeSimpleColors();
void MakeSimpleMatchedColors();

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

void LoadConfig(string filename)
{
	FILE *f = fopen(filename.c_str(), "r");
	if (!f)
		return;

	fscanf(f, "gGridX: %d\n", &gGridX);
	fscanf(f, "gGridY: %d\n", &gGridY);
	fscanf(f, "gDrawGrid: %d\n", &gDrawGrid);
	fscanf(f, "gMakeKey: %d\n", &gMakeKey);
	fscanf(f, "gWinWidth: %d\n", &gWinWidth);
	fscanf(f, "gLegoColorCount: %d\n", &gLegoColorCount);
	int tmp;
	float tx, ty;
	for (int i=0; i <4; i++)
	{
		printf("src_vertices[%d]: %f, %f\n", i, src_vertices[i].x, src_vertices[i].y);
		if (fscanf(f, "src_vertices[%d]: %f, %f\n", &tmp, &tx, &ty) == 3)
		{
			src_vertices[i].x=tx;
			src_vertices[i].y=ty;
		}
		printf("src_vertices[%d]: %f, %f\n", i, src_vertices[i].x, src_vertices[i].y);
	}

}

void SaveConfig(string filename)
{
	FILE *f = fopen(filename.c_str(), "w");
	if (!f)
		return;

	fprintf(f, "gGridX: %d\n", gGridX);
	fprintf(f, "gGridY: %d\n", gGridY);
	fprintf(f, "gDrawGrid: %d\n", gDrawGrid);
	fprintf(f, "gMakeKey: %d\n", gMakeKey);
	fprintf(f, "gWinWidth: %d\n", gWinWidth);
	fprintf(f, "gLegoColorCount: %d\n", gLegoColorCount);
	for (int i=0; i <4; i++)
	{
		fprintf(f, "src_vertices[%d]: %f, %f\n", i, src_vertices[i].x, src_vertices[i].y);
	}
	
}


Scalar getBestColor(Vec3b v)
{
    Scalar color;
    float distance = -1;
    
    for (int i=0; i < keys.size(); i++)
    {
        float d = (keys[i].color[0] - v[0])*(keys[i].color[0] - v[0]) +
                    (keys[i].color[1] - v[1])*(keys[i].color[1] - v[1]) +
                    (keys[i].color[2] - v[2])*(keys[i].color[2] - v[2]);
        
        if (distance < 0 || d < distance)
        {
            
            distance = d;
            color = keys[i].color;
        }
    }
    
    return color;
}

string getBestColorString(Vec3b v)
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
    
    int w = src.cols / gGridX;
    int h = src.rows / gGridY;
    
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
    int w = src.cols / gGridX;
    int h = src.rows / gGridY;
    
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

    Mat timg = Mat(gGridY*h, gGridX *w, CV_8UC3);
    
    
    for (int x=0; x < gGridX; x++)
     for (int y=0; y < gGridY; y++)
    {
        cv::Rect rect(x*w, y*h, w, h);
        
        cv::rectangle(timg, rect, getColorFromGrid(x,y), CV_FILLED);
    }
    
    return timg;
}

void DrawGrid()
{
    // Draw a line
    for (int i=0; i < wimg.cols; i+= (wimg.cols / gGridX))
        line( wimg, Point( i, 0 ), Point( i, wimg.rows), Scalar( 255, 255, 255 ),  2, 8 );
    
    for (int j=0; j < wimg.rows; j+= (wimg.rows / gGridY))
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


Mat CreateMatchedImage(int w=20, int h = 20)
{

    Mat timg = Mat(gGridY*h, gGridX *w, CV_32FC4);

	loadKeys();
	Mat src = MakeSimpleImage(1, 1);
    
    //lets do key matching
    {
        Mat lab_image;
        cv::cvtColor(src, lab_image, CV_BGR2Lab);
    
       
        for( int y = 0; y < lab_image.rows; y++ )
        {
            for( int x = 0; x < lab_image.cols; x++ )
            {
                Vec3b v = lab_image.at<Vec3b>(y,x);
				Scalar s = getBestColor(v);
				

				cv::Rect rect(x*w, y*h, w, h);
        
				cv::rectangle(timg, rect, s, CV_FILLED);
            }
           
        }
       
    }
	Mat rtimg = Mat(gGridY*h, gGridX *w, CV_32FC4);
	cv::cvtColor(timg, rtimg, cv::COLOR_Lab2BGR);


	return rtimg;
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
    
        FILE *f = fopen ((gFilename + ".csv").c_str(), "w");
        for( int y = 0; y < lab_image.rows; y++ )
        {
            for( int x = 0; x < lab_image.cols; x++ )
            {
                Vec3b v = lab_image.at<Vec3b>(y,x);
            
                fprintf(f, "%s,", getBestColorString(v).c_str());
            }
            fprintf(f, "\n");
        }
        fclose(f);
    }
    
    //make a transpose version 
    {
        Mat lab_image;
        cv::cvtColor(src, lab_image, CV_BGR2Lab);
        
        FILE *f = fopen ((gFilename + "-t.csv").c_str(), "w");
        
        for( int x = 0; x < lab_image.cols; x++ )
        {
            for( int y = 0; y < lab_image.rows; y++ )
            {
                Vec3b v = lab_image.at<Vec3b>(y,x);
                
                fprintf(f, "%s,", getBestColorString(v).c_str());
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
    kmeans(samples, gLegoColorCount, labels, TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.0001), attempts, KMEANS_PP_CENTERS, centers );
    
    if (gMakeKey)
    {
        Mat kimg(gLegoColorCount, 1, CV_8UC3);
        
        
        FILE *fk = fopen("key.txt", "w");
        for (int i = 0; i < gLegoColorCount; i++)
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
    kmeans(samples, gLegoColorCount, labels, TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.0001), attempts, KMEANS_PP_CENTERS, centers );
    
    
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

void UpdateWindows()
{
	//update the main window
    Mat boximg = img.clone();
 
    line(boximg,src_vertices[0], src_vertices[1], Scalar(0,255,0), 8);
    line(boximg,src_vertices[1], src_vertices[2], Scalar(0,255,0), 8);
    line(boximg,src_vertices[2], src_vertices[3], Scalar(0,255,0), 8);
    line(boximg,src_vertices[3], src_vertices[0], Scalar(0,255,0), 8);
    
    circle(boximg, src_vertices[targetNum], 8, Scalar(255,255,0), 8);
    
    //show the image
    imshow("Photograph", boximg);

	//update the warp
	FixWarp();

	//update the simple colors
	MakeSimpleColors();

	//match colors
	//TODO: fix this
	//MakeSimpleMatchedColors();

}

void MakeSimpleColors()
{
	simg = MakeSimpleImage();
            
    //Create a window
	if (!cvGetWindowHandle("SimpleColors"))
	{
		namedWindow("SimpleColors", 1);
		int h = img.cols;
		cvMoveWindow("SimpleColors", gWinPad+h, gWinPad);
	}
            
    //show the image
    imshow("SimpleColors", simg);
}

void MakeSimpleMatchedColors()
{
	Mat smimg = CreateMatchedImage();
            
    //Create a window
	if (!cvGetWindowHandle("SimpleMatchedColors"))
	{
		namedWindow("SimpleMatchedColors", 1);
		int h = img.cols;
		cvMoveWindow("SimpleMatchedColors", gWinPad+h, gWinPad);
	}
            
    //show the image
    imshow("SimpleMatchedColors", smimg);
}

void FixWarp()
{
      
    Mat warpMatrix = getPerspectiveTransform(src_vertices, dst_vertices);
    warpPerspective(img, wimg, warpMatrix, wimg.size(), INTER_LINEAR, BORDER_CONSTANT);
    
    if (!cvGetWindowHandle("warp"))
    {
        //Create a window
        namedWindow("warp", 1);
        
        //create sliders?
        createTrackbar( "Grid Size X","warp", &gGridX, 50, on_slider );
        createTrackbar( "Grid Size Y","warp", &gGridY, 50, on_slider );

		cvMoveWindow("warp", gWinWidth+gWinPad, gWinPad);

    }

	if (gDrawGrid)
       DrawGrid();

    //show the image
    imshow("warp", wimg);
    
}
                 


void on_slider( int, void* )
{
    FixWarp();
   
}

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if  ( event == EVENT_LBUTTONDOWN )
    {
        printf("click at %d %d \n", x, y);
        
        //test if we are clicking close to another point
        for (int i=0; i<4; i++)
        {
            if ((src_vertices[i].x - x)*(src_vertices[i].x - x) + (src_vertices[i].y - y)*(src_vertices[i].y - y) < 20*20)
            {
                targetNum=i;
            }
        }
        
        src_vertices[targetNum] = Point(x,y);
        UpdateWindows();
    }
    else if ( event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_LBUTTON)
    {
      src_vertices[targetNum] = Point(x,y);

        UpdateWindows();
        
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

	//load our previous config
	LoadConfig(gConfigFilename);

	Mat oimg = imread(gFilename);
    
    Mat gimg = oimg.clone();
   
    
    gScale =  gWinWidth / float(oimg.cols);
    
    
    Size size(oimg.cols*gScale,oimg.rows*gScale);//the dst image size,e.g.100x100
    
   
    resize(oimg,img,size);//resize image
    int blocksize=60;
    wimg = Mat(gGridX*blocksize, gGridY *blocksize*.7, CV_8UC3);//img.clone();//Mat (200,400, CV_8U);


	//now make the dst rects
    dst_vertices[0] = Point(0, 0);
    dst_vertices[1] = Point(wimg.cols, 0);
    dst_vertices[2] = Point(wimg.cols, wimg.rows);
    dst_vertices[3] = Point(0, wimg.rows);
    
    src_vertices[0] = Point(img.cols*.25, img.rows*.25);
    src_vertices[1] = Point(img.cols*.75, img.rows*.25);
    src_vertices[2] = Point(img.cols*.75, img.rows*.5);
    src_vertices[3] = Point(img.cols*.25, img.rows*.5);


	//load our previous config
	LoadConfig(gConfigFilename);
    

    
   
    
    
    //Create a window
    namedWindow("Photograph", CV_WINDOW_AUTOSIZE);
	
	//move it to open spot
	cvMoveWindow("Photograph", gWinPad, gWinPad);
    
    //set the callback function for any mouse event
    setMouseCallback("Photograph", CallBackFunc, NULL);
    
    //show the image
    UpdateWindows();
   
    
    // Wait until user press some key
    int key = waitKey(0);

    while (key != 27)
    {
        
        key = waitKey(0);
        
        if (key >= '1' && key <= '4')
        {
            targetNum = key -'1';
			
            UpdateWindows();
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
            UpdateWindows();
        }
        
        if (key == 'k')
            Cluster();
        
        if (key == 'i')
        {
           MakeSimpleColors();
        }
        
        int x=0, y=0;
        if (key == 'w') y++; // up
        if (key == 's') y--;   // down
        if (key == 'a') x--;   //  left
        if (key == 'd')  x++;  // right
        
        if (x || y)
        {
            
            src_vertices[targetNum%4] += Point2f(x,y);
            UpdateWindows();
           
        }
        
        if (key == 'o')
        {
            
            optimizeCorners();
            UpdateWindows();
        }
         if (key == 'p')
        {
            WriteCSV();
        }
    }
    
	SaveConfig(gConfigFilename);

    return 0;
}
