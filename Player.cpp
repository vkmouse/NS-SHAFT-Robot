#include "__header__.h"
#include "opencv2/imgcodecs.hpp"

Player::Player(double scale)
{
    this->scale = scale;
    for (int i = 0; i < 4; i++)
    {
        check_gameover_point[i].x = check_gameover_point[i].x * scale;
        check_gameover_point[i].y = check_gameover_point[i].y * scale;
    }
    scene = Rect(ceil(40 * scale),
                 ceil(65 * scale),
                 floor(384 * scale),
                 floor(352 * scale));
    dst_board = Rect(0, 0, 0, 0);
}
void Player::choose_action(int &mode, short &time)
{
    dst_board = Rect();
    // check gameover
    if (is_gameover())
    {
        mode = RESTART;
        return;
    }
    // check number of bboxes
    int num_bboxes = bboxes.size();
    if (num_bboxes < 2) // Unable to detect boards other than the board under the character
    {
        mode = STOP;
        time = 50;
        return;
    }

    // sort by y-axis from lowest to highest
    Rect *Fboard = &(*bboxes.begin()); // first board under the character
    if (character.br().y - Fboard->y > Fboard->height)
        return;

    // check distance reachable
    std::vector<bool> reachable(num_bboxes, true);
    std::vector<double> required_time_x(num_bboxes);
    std::vector<double> required_time_y(num_bboxes);
    reachable[0] = false;
    double speed_x = 0.1732 * scale;          // speed_x = 0.1732 pixels / ms
    double acc_y = 2.0 * scale / 57.0 / 57.0; // acceleration for y-axis is 2 pixels per 57 ms.

    // S = v0t + 1/2 at^2, v0 -> 0
    // S = 1/2 at^2
    // t = sqrt (2S / a)
    for (int i = 1; i < num_bboxes; i++)
    {
        required_time_x[i] = std::max(0, std::max(Fboard->tl().x, bboxes[i].tl().x) - std::min(Fboard->br().x, bboxes[i].br().x)) / speed_x;
        required_time_y[i] = sqrt(2.0 * abs(bboxes[i].y - Fboard->y) / acc_y);
        if (required_time_x[i] > required_time_y[i]) // unreachable
            reachable[i] = false;
    }

    // check number of reachable bboxes
    std::vector<Rect> reachable_bboxes;
    std::vector<double> reachable_time_x;
    std::vector<double> reachable_time_y;
    for (int i = 0; i < num_bboxes; i++)
        if (reachable[i])
        {
            reachable_bboxes.push_back(bboxes[i]);
            reachable_time_x.push_back(required_time_x[i]);
            reachable_time_y.push_back(required_time_y[i]);
        }
    if (reachable_bboxes.size() == 0)
        return;

    // select destination board
    int r = 0;
    for (int i = 0; i < reachable_bboxes.size(); i++)
    {
        if (reachable_bboxes[r].height > reachable_bboxes[i].height ||                             // choose smaller
            reachable_time_y[r] - reachable_time_x[r] < reachable_time_y[i] - reachable_time_x[i]) // choose bigger
            r = i;
    }
    dst_board = reachable_bboxes[r];

    // choose action
    if (dst_board.x > Fboard->x) // move right
    {
        mode = MOVERIGHT;
        reachable_time_x[r] += (Fboard->br().x - character.x) / speed_x;
    }
    else if (dst_board.x < Fboard->x) // move left
    {
        mode = MOVELEFT;
        reachable_time_x[r] += (character.br().x - Fboard->x) / speed_x;
    }
    else // stop
    {
        if (rand() % 2)
        {
            mode = MOVERIGHT;
            reachable_time_x[r] = (Fboard->br().x - character.x) / speed_x / 2;
        }
        else
        {
            mode = MOVELEFT;
            reachable_time_x[r] = (character.br().x - Fboard->x) / speed_x / 2;
        }
    }
    reachable_time_x[r] *= (11.0 / 10.0);
    time = round(std::min(5000.0, std::max(0.0, reachable_time_x[r])));
}

Mat Player::detect_and_draw(Mat observation)
{
    this->observation = observation;
    dst = observation.clone();

    Mat img;
    Scalar color = Scalar(0, 255, 0);

    // Detect character
    // 1. clip the scene of the game screen currently
    observation(scene).copyTo(img);
    // 2. detect the position of the character
    character = detect_character(img);
    character.x += scene.x;
    character.y += scene.y;
    // 3. draw the rectangle of the character position
    rectangle(dst, character.tl(), character.br(), color);

    // Detect object
    // 1. clip the scene of the game screen currently under the character
    Rect scene_under_character = Rect(scene.x,
                                      std::min(scene.br().y - 1, character.br().y),
                                      scene.width,
                                      std::max(1, scene.br().y - character.br().y));
    observation(scene_under_character).copyTo(img);
    // 2. detect the positions of all the objects
    bboxes = detect_object(img);
    for (size_t i = 0; i < bboxes.size(); i++)
    {
        bboxes[i].x += scene_under_character.x;
        bboxes[i].y += scene_under_character.y;
        // 3. draw the rectangles of the positions of all the objects
        rectangle(dst, bboxes[i].tl(), bboxes[i].br(), color);
    }

    color = Scalar(0, 255, 255);
    if (!dst_board.empty())
        rectangle(dst, dst_board.tl(), dst_board.br(), color);
    return dst;
}
Rect Player::detect_character(Mat img)
{
    Mat hsv;
    cvtColor(img, hsv, COLOR_BGR2HSV);

    Mat mask;
    inRange(hsv, Scalar(30, 255, 128), Scalar(30, 255, 255), mask); // yellow

    // findContours
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;
    findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    if (contours.size() == 0) // cannot find the character
        return Rect(0, 0, img.cols, img.rows);

    std::vector<std::vector<Point>> contours_poly(contours.size());
    approxPolyDP(contours[0], contours_poly[0], 1, true);
    Rect character_rect = boundingRect(contours_poly[0]);
    for (size_t i = 1; i < contours.size(); i++)
    {
        approxPolyDP(contours[i], contours_poly[i], 1, true);
        Rect rect = boundingRect(contours_poly[i]);
        if (rect.area() > character_rect.area()) // the most yellow pixels in the area
            character_rect = rect;
    }
    if (character_rect.area() < 4)
        return Rect(0, 0, img.cols, img.rows);
    Point center = (character_rect.tl() + character_rect.br()) / 2;
    // Size(30, 29) is the size of character.
    character_rect.x = center.x - 30 * scale / 2;
    character_rect.y = center.y - 29 * scale / 2;
    character_rect.width = 30 * scale;
    character_rect.height = 29 * scale;

    return character_rect;
}
std::vector<Rect> Player::detect_object(Mat img)
{
    Mat hsv;
    copyMakeBorder(img, img, 0, 0, 1, 1, BORDER_CONSTANT, Scalar(0, 0, 0)); // padding left and right
    cvtColor(img, hsv, COLOR_BGR2HSV);

    Mat bg_mask;
    inRange(hsv, Scalar(0, 0, 0), Scalar(0, 0, 0), bg_mask);
    bitwise_not(bg_mask, bg_mask);

    // find bounding boxes
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;
    findContours(bg_mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    std::vector<std::vector<Point>> contours_poly(contours.size());
    std::vector<Rect> bboxes(contours.size());
    for (size_t i = 0; i < contours.size(); i++)
    {
        approxPolyDP(contours[i], contours_poly[i], 1, true);
        bboxes[i] = boundingRect(contours_poly[i]);
    }

    nms(bboxes, 0.0); // delete the smaller overlapping area

    if (bboxes.size() < 2)
        return bboxes;

    // sort by y-axis from lowest to highest
    std::vector<int> index(bboxes.size());
    for (int i = 0; i < index.size(); ++i)
        index[i] = i;
    sort(index.begin(), index.end(), [&](int a, int b) {
        return bboxes[a].tl().y < bboxes[b].tl().y;
    });
    std::vector<Rect> sort_bboxes;
    for (int i = 0; i < index.size(); i++)
        if (bboxes[index[i]].height >= bboxes[index.front()].height &&
            bboxes[index[i]].width >= bboxes[index.front()].width)
            sort_bboxes.push_back(bboxes[index[i]]);

    return sort_bboxes;
}
bool Player::is_gameover()
{
    Mat gray;
    cvtColor(observation, gray, COLOR_BGRA2GRAY);
    return gray.at<uchar>(check_gameover_point[0]) == 240 &&
           gray.at<uchar>(check_gameover_point[1]) == 240 &&
           gray.at<uchar>(check_gameover_point[2]) == 240 &&
           gray.at<uchar>(check_gameover_point[3]) == 240;
}
double Player::IOU(const Rect &r1, const Rect &r2)
{
    int x1 = std::max(r1.x, r2.x);
    int y1 = std::max(r1.y, r2.y);
    int x2 = std::min(r1.x + r1.width, r2.x + r2.width);
    int y2 = std::min(r1.y + r1.height, r2.y + r2.height);
    int w = std::max(0, (x2 - x1 + 1));
    int h = std::max(0, (y2 - y1 + 1));
    double inter = w * h;
    double o = inter / (r1.area() + r2.area() - inter);
    return (o >= 0) ? o : 0;
}
void Player::nms(std::vector<Rect> &proposals, const double nms_threshold)
{
    std::vector<int> scores;
    for (auto i : proposals)
        scores.push_back(i.area());

    std::vector<int> index;
    for (int i = 0; i < scores.size(); ++i)
    {
        index.push_back(i);
    }

    sort(index.begin(), index.end(), [&](int a, int b) {
        return scores[a] > scores[b];
    });

    std::vector<bool> del(scores.size(), false);
    for (size_t i = 0; i < index.size(); i++)
    {
        if (!del[index[i]])
        {
            for (size_t j = i + 1; j < index.size(); j++)
            {
                if (IOU(proposals[index[i]], proposals[index[j]]) > nms_threshold)
                {
                    del[index[j]] = true;
                }
            }
        }
    }

    std::vector<Rect> new_proposals;
    for (const auto i : index)
    {
        if (!del[i])
            new_proposals.push_back(proposals[i]);
    }
    proposals = new_proposals;
}