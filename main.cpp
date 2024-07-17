#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;


vector<Point2f> sliding_window(Mat image, Rect window)
{
    vector<Point2f> points;
    const Size img_size = image.size();
    bool should_break = false;
    
    while (true)
    {
        float current_x = window.x + window.width * 0.5f;
        
        Mat roi = image(window); // Extract region of interest
        vector<Point2f> locations;
        
        findNonZero(roi, locations); // Get all non-black pixels. All are white in our case
        
        float avg_x = 0.0f;
        
        for (int i = 0; i < locations.size(); ++i) // Calculate average X position
        {
            float x = locations[i].x;
            avg_x += window.x + x;
        }
        
        avg_x = locations.empty() ? current_x : avg_x / locations.size();
        
        Point point(avg_x, window.y + window.height * 0.5f);
        points.push_back(point);
        
        // Move the window up
        window.y -= window.height;
        
        // For the uppermost position
        if (window.y < 0)
        {
            window.y = 0;
            should_break = true;
        }
        
        // Move x position
        window.x += (point.x - current_x);
        
        // Make sure the window doesn't overflow, we get an error if we try to get data outside the matrix
        if (window.x < 0)
            window.x = 0;
        if (window.x + window.width >= img_size.width)
            window.x = img_size.width - window.width - 1;
        
        if (should_break)
            break;
    }
    
    return points;
}


int main() {
    // File
    const string filename = "assets/dashcam.mp4";
    VideoCapture cap(filename);

    if (!cap.isOpened()) {
        cout << "No image" << "\n";
    }

    // Source points
    Point2f src_points[4];
    src_points[0] = Point(527, 514);
    src_points[1] = Point(765, 514);
    src_points[2] = Point(1205, 839);
    src_points[3] = Point(60, 839);
    
    // Destination pointsr
    Point2f dst_points[4];
    dst_points[0] = Point(0, 0);
    dst_points[1] = Point(640, 0);
    dst_points[2] = Point(640, 480);
    dst_points[3] = Point(0, 480);

    // Perspective matrix
    Mat perspect_mtx = getPerspectiveTransform(src_points, dst_points);
    Mat dst(480, 640, CV_8UC3);

    // Invert perspective matrix
    Mat inverted_perspect_mtx;
    invert(perspect_mtx, inverted_perspect_mtx);

    Mat org_img;
    Mat img;
    Mat resized_img;

    while (true) {
        cap.read(org_img);
        if (org_img.empty()) break;

        // BEV image
        warpPerspective(org_img, dst, perspect_mtx, dst.size(), INTER_LINEAR, BORDER_CONSTANT);

        // // Draw circles on each point
        // for (int i = 0; i < 4; ++i) {
        //     circle(org_img, src_points[i], 10, Scalar(0, 0, 255), -1);  // Draw a filled red circle
        // }

        cvtColor(dst, img, COLOR_RGB2GRAY);

        // Extract yellow and white info from img
        Mat mask_yellow, mask_white;

        inRange(img, Scalar(20, 100, 100), Scalar(30, 255, 255), mask_yellow);
        inRange(img, Scalar(150, 150, 150), Scalar(255, 255, 255), mask_white);

        Mat mask, processed;
        bitwise_or(mask_yellow, mask_white, mask); // Combine both masks
        bitwise_and(img, mask, processed); // Extrect what matches

        // Blur img so that gaps are smoother
        const Size kernel_size = Size(9, 9);
        GaussianBlur(processed, processed, kernel_size, 0);

        // Fill the gaps
        Mat kernel = Mat::ones(15, 15, CV_8U);
        dilate(processed, processed, kernel);
        erode(processed, processed, kernel);
        morphologyEx(processed, processed, MORPH_CLOSE, kernel);

        // Apply threshold
        const int threshold_val = 110;
        threshold(processed, processed, threshold_val, 255, THRESH_BINARY);

        Mat hist;
        int histSize = 256;
        float range[] = { 0, 256 }; //the upper boundary is exclusive
        const float* histRange = { range };
        calcHist(&processed, 1, 0, Mat(), hist, 1, &histSize, &histRange, true);
        
        // Get points for left sliding window
        vector<Point2f> pts = sliding_window(processed, Rect(0, 420, 120, 60));
        vector<Point> all_pts;
        
        // Transform points back into original image space
        vector<Point2f> out_pts;
        perspectiveTransform(pts, out_pts, inverted_perspect_mtx); 
        
        // Draw points
        for (int i = 0; i < out_pts.size() - 1; ++i)
        {
            line(org_img, out_pts[i], out_pts[i + 1], Scalar(30, 255, 255), 3);
            all_pts.push_back(Point(out_pts[i].x, out_pts[i].y));
        }
        
        all_pts.push_back(Point(out_pts[out_pts.size() - 1].x, out_pts[out_pts.size() - 1].y));
        
        Mat out;
        cvtColor(processed, out, COLOR_GRAY2BGR);
        
        // Draw line on processed image
        for (int i = 0; i < pts.size() - 1; ++i) 
            line(out, pts[i], pts[i + 1], Scalar(255, 0, 0));
        
        // Sliding window for right side
        pts = sliding_window(processed, Rect(500, 420, 140, 60));
        perspectiveTransform(pts, out_pts, inverted_perspect_mtx);
        
        // Draw other lane and append points
        for (int i = 0; i < out_pts.size() - 1; ++i)
        {
            line(org_img, out_pts[i], out_pts[i + 1], Scalar(30, 255, 255), 3);
            all_pts.push_back(Point(out_pts[out_pts.size() - i - 1].x, out_pts[out_pts.size() - i - 1].y));
        }
        
        all_pts.push_back(Point(out_pts[0].x - (out_pts.size() - 1) , out_pts[0].y));
        
        for (int i = 0; i < pts.size() - 1; ++i)
            line(out, pts[i], pts[i + 1], Scalar(0, 0, 255));

        // Overlay
        vector<vector<Point>> arr;
        arr.push_back(all_pts);
        Mat overlay = Mat::zeros(org_img.size(), org_img.type());
        fillPoly(overlay, arr, Scalar(255, 0, 0));
        addWeighted(org_img, 1, overlay, 0.4, 0, org_img);
        
        // Show
        resize(org_img, resized_img, Size(), 0.5, 0.5);
        imshow("Preprocess", out);
        imshow("src", resized_img);
        
        if (waitKey(50) > 0)
            break;
    }

    cap.release();
    
    waitKey();
    return 0;
}