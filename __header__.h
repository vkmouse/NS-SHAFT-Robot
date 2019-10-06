#include "opencv2/highgui.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>
#include <windows.h>

using namespace cv;

#define MOVELEFT 0
#define MOVERIGHT 1
#define STOP 2
#define RESTART 3

class Environment
{
public:
    Environment(HWND hwnd, double scale);
    Mat get_observation();

private:
    double scale;
    HWND hwnd;
    Mat hwnd2mat(HWND hwnd);
};
class Actor
{
public:
    Actor(HWND hwnd);
    void action(int mode, short time);
    DWORD get_status();

private:
    HWND hwnd;
    HANDLE handle;
    int mode;
    short time;

    void move_left(short time);
    void move_right(short time);
    void restart();
    void stop(short time);
    static DWORD WINAPI s_run(LPVOID p)
    {
        return ((Actor *)p)->run(); // function pointer
    }
    DWORD run();
};
class Player
{
public:
    Player(double scale);
    void choose_action(int &mode, short &time);
    Mat detect_and_draw(Mat img);

private:
    double scale;
    Point check_gameover_point[4] = {Point(165, 107),
                                     Point(467, 107),
                                     Point(165, 324),
                                     Point(467, 324)};
    Rect character;
    std::vector<Rect> bboxes;
    Rect scene;
    Rect dst_board;
    Mat dst;
    Mat observation;

    Rect detect_character(Mat img);
    std::vector<Rect> detect_object(Mat img);
    bool is_gameover();
    double IOU(const Rect &r1, const Rect &r2);
    void nms(std::vector<Rect> &proposals, const double nms_threshold);
    void check_distance_reachable(std::vector<Rect> &bboxes, std::vector<double> &time_x, std::vector<double> &time_y);
};