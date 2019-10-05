#include "__header__.h"
#include <Tchar.h> // for the _T type

int main(int argc, char *argv[])
{
    HWND hwnd = FindWindow(NULL, _T("NS-SHAFT"));
    double scale = 1.0;
    Environment env(hwnd, scale);
    Actor act(hwnd);
    Player player(scale);

    while (waitKey(1000 / 60) != 27) // 27 is esc
    {
        Mat src = env.get_observation();
        Mat dst = player.detect_and_draw(src);

        if (act.get_status() != STILL_ACTIVE)
        {
            // choose action
            int mode = STOP;
            short time = 0;
            player.choose_action(mode, time);
            // execute action
            act.action(mode, time);
        }
        imshow("output", dst);
    }

    return 0;
}