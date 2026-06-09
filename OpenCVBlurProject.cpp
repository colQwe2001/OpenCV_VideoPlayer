#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm> 
#include <cstdlib>
#include <chrono>
#include <windows.h>
#include <filesystem>
#include "Constants.h"
using namespace UI;

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

bool isPaused = false;
cv::Point oldMousePos(0,0);
cv::Point mousePos;
bool mouseClicked = false;
bool SpeedMenuActive = false;
std::string Name;
ma_engine audioEngine;
ma_sound audioSound;
bool audioInitialized = false;
std::string command ;
std::string Speed_Text = "1x";
int line_width = 12.5;
int Speed_Text_X = 14;
bool CurrentTime = true;
int TimeStringLenght = 60;
float volume = 1.0f;
float save_volume;
bool is_silent = false;
bool featuresActive = false;
bool converted = false;
int SleepTimer = 0;
bool IsSleep = false;
auto startTimer = std::chrono::steady_clock::now();
bool loopActive = false;

void GetScreen(const cv::Mat& frameScreen);

class DrawSpecificFigure
{
private:
    int x;
    int y;
    int width;
    int height;
public:
    DrawSpecificFigure(int x, int y, int width, int height) :
        x(x), y(y), width(width), height(height) {
    }
    void DrawRoundedRectangle(cv::Mat& img, const cv::Scalar& figureColor, int radius) {
        if (radius <= 0) {
            cv::rectangle(img, cv::Point(x, y), cv::Point(x + width, y + height),
                figureColor, -1, cv::LINE_AA);
        }
        else {
            cv::rectangle(img, cv::Point(x, y - radius), cv::Point(x + width, y + height + radius),
                figureColor, -1, cv::LINE_AA);
            cv::rectangle(img, cv::Point(x - radius, y), cv::Point(x + width + radius, y + height),
                figureColor, -1, cv::LINE_AA);
            cv::ellipse(img, cv::Point(x, y), cv::Size(radius, radius), 0, 180, 270, figureColor, -1, cv::LINE_AA);
            cv::ellipse(img, cv::Point(x + width, y), cv::Size(radius, radius), 0, 270, 360, figureColor, -1, cv::LINE_AA);
            cv::ellipse(img, cv::Point(x + width, y + height), cv::Size(radius, radius), 0, 0, 90, figureColor, -1, cv::LINE_AA);
            cv::ellipse(img, cv::Point(x, y + height), cv::Size(radius, radius), 0, 90, 180, figureColor, -1, cv::LINE_AA);
        }
    }
};

class DrawInterface
{
private:
    int Xposition;
    int Yposition;
    cv::VideoCapture& cap;
public:
    DrawInterface(int Xposition, int Yposition, cv::VideoCapture& cap) :
        Xposition(Xposition), Yposition(Yposition), cap(cap) {
       
    }
    void DrawProgressBar(cv::Mat& resultWin, const cv::Rect& sizeOfWindow, const int currentFrame, const int totalFrames, int fps, cv::VideoCapture& cap) {
        int barY = sizeOfWindow.height - PROGRESS_Y_OFFSET;
        int barWidth = sizeOfWindow.width - PROGRESS_WIDTH_OFFSET;
        bool isOverBar = (mousePos.y > barY - PROGRESS_HEIGHT - 5 && mousePos.y < barY + 5 && mousePos.x > PROGRESS_X && mousePos.x < PROGRESS_X + barWidth);
        cv::rectangle(resultWin,
            cv::Point(PROGRESS_X, barY - PROGRESS_HEIGHT / 2),
            cv::Point(PROGRESS_X + barWidth, barY + PROGRESS_HEIGHT / 2),
            cv::Scalar(60, 60, 60), -1);
        int progressWidth = (int)((currentFrame / (double)totalFrames) * barWidth);
        cv::rectangle(resultWin,
            cv::Point(PROGRESS_X, barY - PROGRESS_HEIGHT / 2),
            cv::Point(PROGRESS_X + progressWidth, barY + PROGRESS_HEIGHT / 2),
            cv::Scalar(UI_COLOR), -1);
        int circlePosX = (int)progressWidth + PROGRESS_X;
        cv::circle(resultWin, cv::Point(circlePosX, barY), HANDLE_RADIUS, cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        if (mouseClicked && isOverBar && SpeedMenuActive == false)
        {
            float clickPercent = (mousePos.x - PROGRESS_X) / (float)barWidth;
            int newFrame = std::clamp((int)(totalFrames * clickPercent), 0, totalFrames - 1);
            cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
            if (audioInitialized) {
                ma_sound_seek_to_second(&audioSound, newFrame / fps);
            }
            mouseClicked = false;
        }
    }
    void InfoButton(cv::Mat& resultWin, const cv::Rect& sizeOfWindow) {
        int btnX = sizeOfWindow.width / 2 - BTN_INFO_OFFSET;
        int btnY = sizeOfWindow.height - 37.5;
        cv::putText(resultWin, "i",
            cv::Point(btnX, btnY),
            FONT, FONT_SIZE_MEDIUM,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        float dist = std::hypot(mousePos.x - (btnX + 5), mousePos.y - (btnY - 7));
        if (mouseClicked && dist <= BTN_RADIUS_SMALL) {
            featuresActive = !featuresActive;
            mouseClicked = false;
        }
        else if (dist <= BTN_RADIUS_SMALL) {
            cv::circle(resultWin, cv::Point(btnX + 3, btnY - 8), BTN_RADIUS_SMALL, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
    }
    void ScreenshotButton(cv::Mat& resultWin, const cv::Rect& sizeOfWindow, cv::Mat& frame) {
        int ButtonPosX = sizeOfWindow.width / 2 - BTN_SCREENSHOT_OFFSET;
        int ButtonPosY = sizeOfWindow.height - 40;
        int dx = mousePos.x - (ButtonPosX + 0);
        int dy = mousePos.y - (ButtonPosY - 5);
        float distanceCamera = std::hypot(dx, dy);
        // Корпус
        DrawSpecificFigure Camera(ButtonPosX - 10, ButtonPosY - 10, 20, 10);
        Camera.DrawRoundedRectangle(resultWin, UI_COLOR, 2);
        DrawSpecificFigure CameraUp(ButtonPosX - 3, ButtonPosY - 13, 6, 3);
        CameraUp.DrawRoundedRectangle(resultWin, UI_COLOR, 1);
        // Объектив
        cv::circle(resultWin, cv::Point(ButtonPosX, ButtonPosY - 5), 5,
        cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        //Вспышка
        cv::rectangle(resultWin,
        cv::Point(ButtonPosX + 7, ButtonPosY - 8),
        cv::Point(ButtonPosX + 10, ButtonPosY - 10),
        cv::Scalar(0, 0, 0), -1, cv::LINE_AA);

        if (mouseClicked && distanceCamera <= BTN_RADIUS_BASIC) {
             GetScreen(frame);
             mouseClicked = false;
        }
        else if (distanceCamera <= BTN_RADIUS_BASIC) {
            cv::circle(resultWin, cv::Point(ButtonPosX, sizeOfWindow.height - 45), BTN_RADIUS_BASIC, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
    }
    void ReloadButton(bool direction, cv::Mat& resultWin, int totalFrames, int fps) {
        if (direction == 0) {
            int ButtonPosX = Xposition - BTN_RELOAD_OFFSET;
            int ButtonPosY = Yposition;
            cv::rectangle(resultWin,
                cv::Point(ButtonPosX + 1, ButtonPosY + 2.5),
                cv::Point(ButtonPosX + 4, ButtonPosY + 17.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            cv::Point triangle[3];
            triangle[0] = cv::Point(ButtonPosX + 18, ButtonPosY + 2.5);
            triangle[1] = cv::Point(ButtonPosX + 18, ButtonPosY + 17.5);
            triangle[2] = cv::Point(ButtonPosX + 6, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangle, 3,
                cv::Scalar(UI_COLOR), cv::LINE_AA);
            int dx = mousePos.x - (ButtonPosX + 10);
            int dy = mousePos.y - (ButtonPosY + 10);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (mouseClicked && distance <= BTN_RADIUS_BASIC) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (audioInitialized) ma_sound_seek_to_second(&audioSound, 0);
                mouseClicked = false;
            }
            else if (distance <= BTN_RADIUS_BASIC) {
                cv::circle(resultWin, cv::Point(ButtonPosX + 10, ButtonPosY + 10), BTN_RADIUS_BASIC,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
        }
        else{
            int ButtonPosX = Xposition + BTN_RELOAD_OFFSET;
            int ButtonPosY = Yposition;
            cv::rectangle(resultWin,
                cv::Point(ButtonPosX - 4, ButtonPosY + 2.5),
                cv::Point(ButtonPosX - 1, ButtonPosY + 17.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            cv::Point triangleE[3];
            triangleE[0] = cv::Point(ButtonPosX - 18, ButtonPosY + 2.5);
            triangleE[1] = cv::Point(ButtonPosX - 18, ButtonPosY + 17.5);
            triangleE[2] = cv::Point(ButtonPosX - 6, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangleE, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            int dx = mousePos.x - (ButtonPosX - 10);
            int dy = mousePos.y - (ButtonPosY + 10);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (mouseClicked && distance <= 20) {
                cap.set(cv::CAP_PROP_POS_FRAMES, totalFrames - 1);
                if (audioInitialized) ma_sound_seek_to_second(&audioSound, totalFrames / fps);
                mouseClicked = false;
            }
            else if (distance <= 20) {
                cv::circle(resultWin, cv::Point(ButtonPosX - 10, ButtonPosY + 10), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
        }
    }
    void RewindButton(bool direction, cv::Mat& resultWin, const int currentFrame, const int framesIn10Seconds, double fps) {
        if (direction == 0) {
            int ButtonPosX = Xposition - BTN_REWIND_OFFSET;
            int ButtonPosY = Yposition;
            cv::Point triangleBack[3];
            triangleBack[0] = cv::Point(ButtonPosX + 20, ButtonPosY + 2.5);
            triangleBack[1] = cv::Point(ButtonPosX + 20, ButtonPosY + 17.5);
            triangleBack[2] = cv::Point(ButtonPosX + 8, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangleBack, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            cv::Point triangleBack2[3];
            triangleBack2[0] = cv::Point(ButtonPosX + 8, ButtonPosY + 2.5);
            triangleBack2[1] = cv::Point(ButtonPosX + 8, ButtonPosY + 17.5);
            triangleBack2[2] = cv::Point(ButtonPosX - 4, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangleBack2, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            int dx = mousePos.x - (ButtonPosX + 10);
            int dy = mousePos.y - (ButtonPosY + 10);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (mouseClicked && distance <= BTN_RADIUS_BASIC) {
                int newFrame = std::max(currentFrame - framesIn10Seconds, 0);
                cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
                if (audioInitialized) {
                    double newTime = newFrame / fps;
                    ma_sound_seek_to_second(&audioSound, newTime);
                }
                mouseClicked = false;
            }
            else if (distance <= BTN_RADIUS_BASIC) {
                cv::circle(resultWin, cv::Point(ButtonPosX + 10, ButtonPosY + 10), BTN_RADIUS_BASIC,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
        }
        else {
            int ButtonPosX = Xposition + BTN_REWIND_OFFSET;
            int ButtonPosY = Yposition;
            cv::Point triangleFor[3];
            triangleFor[0] = cv::Point(ButtonPosX - 8, ButtonPosY + 2.5);
            triangleFor[1] = cv::Point(ButtonPosX - 8, ButtonPosY + 17.5);
            triangleFor[2] = cv::Point(ButtonPosX + 4, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangleFor, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            cv::Point triangleFor2[3];
            triangleFor2[0] = cv::Point(ButtonPosX - 20, ButtonPosY + 2.5);
            triangleFor2[1] = cv::Point(ButtonPosX - 20, ButtonPosY + 17.5);
            triangleFor2[2] = cv::Point(ButtonPosX - 8, ButtonPosY + 10);
            cv::fillConvexPoly(resultWin, triangleFor2, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            int dx = mousePos.x - (ButtonPosX - 10);
            int dy = mousePos.y - (ButtonPosY + 10);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (mouseClicked && distance <= 20) {
                int newFrame = std::max(currentFrame + framesIn10Seconds, 0);
                cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
                if (audioInitialized) {
                    double newTime = newFrame / fps;
                    ma_sound_seek_to_second(&audioSound, newTime);
                }
                mouseClicked = false;
            }
            else if (distance <= 20) {
                cv::circle(resultWin, cv::Point(ButtonPosX - 10, ButtonPosY + 10), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
        }
    }
    void PauseButton(cv::Mat& resultWin, bool& isPaused) {
        int ButtonPosX = Xposition - 15;
        int ButtonPosY = Yposition - 5;
        int dx = mousePos.x - (ButtonPosX + 12.5);
        int dy = mousePos.y - (ButtonPosY + 12.5);
        float distance = std::sqrt(dx * dx + dy * dy);
        if (isPaused) {
            cv::Point pts[3];
            pts[0] = cv::Point(ButtonPosX + 10, ButtonPosY + 7.5);
            pts[1] = cv::Point(ButtonPosX + 10, ButtonPosY + 22.5);
            pts[2] = cv::Point(ButtonPosX + 25, ButtonPosY + 15);
            cv::fillConvexPoly(resultWin, pts, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
        }
        else {
            cv::rectangle(resultWin,
                cv::Point(ButtonPosX + 7.5, ButtonPosY + 7.5),
                cv::Point(ButtonPosX + 12.5, ButtonPosY + 22.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            cv::rectangle(resultWin,
                cv::Point(ButtonPosX + 17.5, ButtonPosY + 7.5),
                cv::Point(ButtonPosX + 22.5, ButtonPosY + 22.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        }

        if (mouseClicked && distance <= BTN_RADIUS_PAUSE) {
            isPaused = !isPaused;
            if (audioInitialized) {
                if (isPaused) ma_sound_stop(&audioSound);
                else ma_sound_start(&audioSound);
            }
            mouseClicked = false;
        }
        else if (distance <= BTN_RADIUS_PAUSE) {
            cv::circle(resultWin, cv::Point(Xposition, Yposition + 10), BTN_RADIUS_PAUSE,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
    }
    void SpeedButton(const cv::Rect& sizeOfWindow, cv::Mat& resultWin) {
        int ButtonPosX = sizeOfWindow.width / 2 + BTN_SPEED_OFFSET;
        int ButtonPosY = sizeOfWindow.height - 45;
        int dx = mousePos.x - ButtonPosX;
        int dy = mousePos.y - ButtonPosY;
        float distanceSpeed = std::sqrt(dx * dx + dy * dy);

        int XspeedPos = sizeOfWindow.width / 2 + 300 - 35;
        int YspeedPos = sizeOfWindow.height - 45 - 35;

        if (mouseClicked && distanceSpeed <= 20) {
            mouseClicked = false;
            SpeedMenuActive = !SpeedMenuActive;
        }
        int underline_center;
        if (Speed_Text == "1.5x" || Speed_Text == "0.5x") {
            line_width = 22.5;
            underline_center = sizeOfWindow.width / 2 + 287.5;
        }
        else {
            line_width = 12.5;
            underline_center = sizeOfWindow.width / 2 + 279;
        }
        if (distanceSpeed <= 20) {
            cv::rectangle(resultWin,
                cv::Point(underline_center - line_width, sizeOfWindow.height - 45 + 11.5),
                cv::Point(underline_center + line_width, sizeOfWindow.height - 45 + 12),
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        cv::putText(resultWin, Speed_Text,
            cv::Point(ButtonPosX - Speed_Text_X, ButtonPosY + 6),
            FONT, FONT_SIZE_SMALL,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
    void VolumeBar(const cv::Rect& sizeOfWindow, cv::Mat& resultWin, int volumeBarX, int volumeBarY) {
        int progressVolume = (int)(VOLUME_BAR_WIDTH * volume);
        cv::rectangle(resultWin, cv::Point(volumeBarX, volumeBarY), cv::Point(volumeBarX + VOLUME_BAR_WIDTH, volumeBarY + VOLUME_BAR_HEIGHT), cv::Scalar(60, 60, 60), -1, cv::LINE_AA);
        cv::rectangle(resultWin, cv::Point(volumeBarX, volumeBarY), cv::Point(volumeBarX + progressVolume, volumeBarY + VOLUME_BAR_HEIGHT), cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        if (mouseClicked &&
            mousePos.x >= volumeBarX && mousePos.x <= volumeBarX + VOLUME_BAR_WIDTH &&
            mousePos.y >= volumeBarY && mousePos.y <= volumeBarY + VOLUME_BAR_HEIGHT) {

            float clickPercent = (mousePos.x - volumeBarX) / (float)VOLUME_BAR_WIDTH;
            volume = std::max(0.0f, std::min(1.0f, clickPercent));
            mouseClicked = false;
        }
    }
    void VolumeButton(cv::Mat& resultWin, int volumeBarX, int volumeBarY) {
        cv::rectangle(resultWin, cv::Point(volumeBarX - 37.5, volumeBarY - 2.5), cv::Point(volumeBarX - 30, volumeBarY + 7.5), cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        cv::Point pts[3] = {
        cv::Point(volumeBarX - 35, volumeBarY + 2.5),
        cv::Point(volumeBarX - 25, volumeBarY + 12.5),
        cv::Point(volumeBarX - 25, volumeBarY - 7.5)
        };
        cv::fillConvexPoly(resultWin, pts, 3, cv::Scalar(UI_COLOR));
        if (volume > 0.8)
            cv::ellipse(resultWin, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                cv::Size(10, 12), 0, 270, 450,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        if (volume > 0.4)
            cv::ellipse(resultWin, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                cv::Size(6, 8), 0, 270, 450,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        if (volume > 0)
            cv::ellipse(resultWin, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                cv::Size(2, 4), 0, 270, 450,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        if (volume < 0.01) {
            cv::line(resultWin,
                cv::Point(volumeBarX - 20, volumeBarY - 2.5),
                cv::Point(volumeBarX - 10, volumeBarY + 7.5),
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            cv::line(resultWin,
                cv::Point(volumeBarX - 20, volumeBarY + 7.5),
                cv::Point(volumeBarX - 10, volumeBarY - 2.5),
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        if (mouseClicked &&
            mousePos.x > volumeBarX - 40 && mousePos.x < volumeBarX - 10 &&
            mousePos.y > volumeBarY - 10 && mousePos.y < volumeBarY + 15 && is_silent == false) {
            save_volume = volume;
            volume = 0.0f;
            mouseClicked = false;
            is_silent = true;
        }
        if (mouseClicked &&
            mousePos.x > volumeBarX - 40 && mousePos.x < volumeBarX - 10 &&
            mousePos.y > volumeBarY - 10 && mousePos.y < volumeBarY + 15 && is_silent == true) {
            volume = save_volume;
            mouseClicked = false;
            is_silent = false;
        }
    }
    void Time(cv::Mat& resultWin, const cv::Rect& sizeOfWindow, int remainingTime, int currentTimeMinutes, int currentTimeSeconds, int totalTimeMinutes, int totalTimeSeconds) {
        //Время видео
        int TimeCenterX = sizeOfWindow.width - TIME_OFFSET_X;
        int TimeCenterY = sizeOfWindow.height - TIME_OFFSET_Y;
        if (mouseClicked && (mousePos.x > TimeCenterX + 0 && mousePos.x < TimeCenterX + TimeStringLenght) && (mousePos.y > TimeCenterY - 20 && mousePos.y < TimeCenterY + 5)) {
            CurrentTime = !CurrentTime;
            mouseClicked = false;
        }
        if (CurrentTime == true) {
            cv::putText(resultWin, std::to_string(currentTimeMinutes) + ":" + std::to_string(currentTimeSeconds) + "/" + std::to_string(totalTimeMinutes) + ":" + std::to_string(totalTimeSeconds),
                cv::Point(TimeCenterX, sizeOfWindow.height - 40),
                FONT, FONT_SIZE_TIME,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        else {
            cv::putText(resultWin, "-" + std::to_string(remainingTime / 60) + ":" + std::to_string(remainingTime % 60) + "/" + std::to_string(totalTimeMinutes) + ":" + std::to_string(totalTimeSeconds),
                cv::Point(TimeCenterX, sizeOfWindow.height - 40),
                FONT, FONT_SIZE_TIME,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        if (CurrentTime == true) {
            TimeStringLenght = 60;
        }
        if (CurrentTime == true && currentTimeSeconds >= 10) {
            TimeStringLenght = 70;
        }
        if (CurrentTime == true && currentTimeMinutes >= 10) {
            TimeStringLenght = 70;
        }
        if (CurrentTime == true && currentTimeMinutes >= 10 && currentTimeSeconds >= 10) {
            TimeStringLenght = 80;
        }
        if (CurrentTime == false) {
            TimeStringLenght = 80;
        }
        if (CurrentTime == false && (remainingTime / 60) >= 10) {
            TimeStringLenght = 90;
        }
        if (CurrentTime == false && (remainingTime % 60) >= 10) {
            TimeStringLenght = 90;
        }
        if (CurrentTime == false && (remainingTime % 60) >= 10 && (remainingTime / 60) >= 10) {
            TimeStringLenght = 100;
        }
    }
    void DrawBouncingBar(const cv::Rect& sizeOfWindow, cv::Mat& resultWin, bool& isPaused) {
        int slidersX = sizeOfWindow.width - BOUSING_BAR_OFFSET;
        static float volumeJumpY = 0;
        static float volumeJumpY2 = 5;
        static float volumeJumpY3 = 2;
        static float volumeJumpDir = 0.1;
        int slidersWidth = 4;
        if (!isPaused) {
            volumeJumpY += volumeJumpDir;
            volumeJumpY2 += volumeJumpDir / 2;
            volumeJumpY3 += volumeJumpDir * 1.5;
        }
        if (volumeJumpY >= 20 || volumeJumpY <= 0) {
            volumeJumpDir = -volumeJumpDir;
        }
        cv::rectangle(resultWin,
            cv::Point(slidersX, sizeOfWindow.height - 40 - volumeJumpY / 2),
            cv::Point(slidersX - slidersWidth, sizeOfWindow.height - 40),
            cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        cv::rectangle(resultWin,
            cv::Point(slidersX + slidersWidth + 2, sizeOfWindow.height - 40 - volumeJumpY2),
            cv::Point(slidersX + 2, sizeOfWindow.height - 40),
            cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
        cv::rectangle(resultWin,
            cv::Point(slidersX + (slidersWidth * 2) + 4, sizeOfWindow.height - 40 - volumeJumpY3 / 1.5),
            cv::Point(slidersX + slidersWidth + 4, sizeOfWindow.height - 40),
            cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
    }
    void LoopButton(cv::Mat& resultWin, const cv::Rect& sizeOfWindow) {
        int ButtonPosX = sizeOfWindow.width - BTN_LOOP_OFFSET;
        int ButtonPosY = sizeOfWindow.height - 45;
        int dx = mousePos.x - ButtonPosX;
        int dy = mousePos.y - ButtonPosY;
        float distance = std::sqrt(dx * dx + dy * dy);
        cv::ellipse(resultWin, cv::Point(ButtonPosX + 1, ButtonPosY),
            cv::Size(7, 5), 0, 270, 360,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        cv::line(resultWin,
            cv::Point(ButtonPosX + 1, ButtonPosY - 5),
            cv::Point(ButtonPosX - 6, ButtonPosY - 5),
            cv::Scalar(UI_COLOR), 1.5, cv::LINE_AA);
        cv::Point arrow[3];
        arrow[0] = cv::Point(ButtonPosX - 6, ButtonPosY - 7);
        arrow[1] = cv::Point(ButtonPosX - 9, ButtonPosY - 5);
        arrow[2] = cv::Point(ButtonPosX - 6, ButtonPosY - 3);
        cv::fillConvexPoly(resultWin, arrow, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
        cv::ellipse(resultWin, cv::Point(ButtonPosX - 1, ButtonPosY),
            cv::Size(7, 5), 0, 180, 90,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        cv::line(resultWin,
            cv::Point(ButtonPosX - 1, ButtonPosY + 5),
            cv::Point(ButtonPosX + 6, ButtonPosY + 5),
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        cv::Point arrow2[3];
        arrow2[0] = cv::Point(ButtonPosX + 6, ButtonPosY + 7);
        arrow2[1] = cv::Point(ButtonPosX + 9, ButtonPosY + 5);
        arrow2[2] = cv::Point(ButtonPosX + 6, ButtonPosY + 3);
        cv::fillConvexPoly(resultWin, arrow2, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
        if (loopActive == true) {
            cv::circle(resultWin, cv::Point(ButtonPosX, ButtonPosY), BTN_RADIUS_SMALL, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        if (mouseClicked && distance <= BTN_RADIUS_SMALL) {
            loopActive = !loopActive;
            mouseClicked = false;
        }
        else if (distance <= BTN_RADIUS_SMALL) {
            cv::circle(resultWin, cv::Point(ButtonPosX, ButtonPosY), BTN_RADIUS_SMALL, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
    }
    void Name(cv::Mat& resultWin, const cv::Rect& sizeOfWindow, std::string Name, std::string OldName, std::string& FinalName) {
        if (converted == false) {
            FinalName = Name.substr(Name.find_last_of("\\/") + 1);
        }
        else {
            FinalName = OldName.substr(OldName.find_last_of("\\/") + 1);
        }
        if (FinalName.length() < 10) {
            cv::putText(resultWin, FinalName, cv::Point(50, sizeOfWindow.height - 37.5), FONT, 0.9, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        else {
            cv::putText(resultWin, FinalName.substr(0, 13) + ".." + FinalName.substr(FinalName.find_last_of(".")),
                cv::Point(50, sizeOfWindow.height - 37.5), FONT, 0.9, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        int NameIndex = FinalName.find_last_of(".");
    }
};
//структура для хранения элементов меню скорости
struct SpeedMenuItem {
    const char* TEXT;
    int delay;
    float speed;
    int yOffset; // смещение по оси у 
};
// обработчик нажатия ЛКМ
void onMouse(int event, int x, int y, int, void*) {
    mousePos = cv::Point(x, y);
    if (event == cv::EVENT_LBUTTONDOWN) {
        mousePos = cv::Point(x, y);
        mouseClicked = true;
    }
}
//Функция скриншота
void GetScreen(const cv::Mat& frameScreen) {
    std::time_t now = std::time(nullptr); // получаем количество секунд с 1970
    struct tm* timeinfo = std::localtime(&now); // разбиваем их в читаемом формате
    char filename[256];
    strftime(filename, sizeof(filename), "Screenshots/screenshot_%Y_%m_%d_%H-%M-%S.png", timeinfo); 
    cv::imwrite(filename, frameScreen); 
    std::cout << "Скриншот сохранён: " << filename << std::endl;
}
//Функция конвертации
std::string ConvertWEBMtoMP4(std::string inputFile) {
    if (!std::filesystem::exists(inputFile)) {
        std::cerr << "Ошибка: файл не существует: " << inputFile << std::endl;
        return "";
    }
    if (inputFile.substr(inputFile.find_last_of(".")) != ".webm") {
        std::cerr << "Ошибка: файл недопустимого формата: " << inputFile << std::endl;
        return "";
    }
    std::string mp4Name = inputFile.substr(0, inputFile.find_last_of(".")) + ".mp4";
    if (std::filesystem::exists(mp4Name)) {
        std::cout << "MP4 файл уже существует, удалите его для переконвертации" << std::endl;
        return mp4Name;
    }
    std::string cmd = "ffmpeg -i \"" + inputFile + "\" -c:v libx264 -c:a aac \"" + mp4Name + "\" -y -loglevel quiet";
    std::cout << "Конвертирую WebM в MP4..." << std::endl;
    system(cmd.c_str());
    if (system(cmd.c_str()) != 0) {
        std::cerr << "Ошибка: ffmpeg завершился с ошибкой" << std::endl;
        return "";
    }
    if (!std::filesystem::exists(mp4Name)) {
        std::cerr << "Ошибка: MP4 файл не создан!" << std::endl;
        return "";
    }
    inputFile = mp4Name;
    return inputFile;
}
//Функция получения кодека
std::string GetCodec(cv::VideoCapture& cap){
    int fourcc = cap.get(cv::CAP_PROP_FOURCC);
    char codec[5] = {
        (char)(fourcc & 0xFF),
        (char)((fourcc >> 8) & 0xFF),
        (char)((fourcc >> 16) & 0xFF),
        (char)((fourcc >> 24) & 0xFF),
        0
    };
    return std::string(codec);
}
//Функция установки иконки
void SetIcon(const std::string& WindowName,const std::string& IconFileName) {
    HWND hwnd = FindWindowA(NULL, WindowName.c_str());  // берём активное окно
    if (hwnd) {
        HICON hIcon = (HICON)LoadImageA(NULL, IconFileName.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            std::cout << "Иконка загружена из файла!" << std::endl;
        }
        else {
            std::cout << "Файл app_icon.ico не найден или не является иконкой" << std::endl;
        }
    }
}
//Функция установки кодека
std::string SetCodec(std::string codecStr, std::string VideoName, std::string OldVideoName) {
    std::string codecStrPrint;
    if (converted == false) {
        std::string ext = VideoName.substr(VideoName.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == "ts" || ext == "m2ts" || ext == "mts") {
            return "MPEG-2 Transport Stream (H.264/H.265)";
        }
        if (ext == "mkv") {
            return "Matroska (MKV)";
        }
        if (ext == "mov") {
            return "QuickTime (H.264/ProRes)";
        }
        if (ext == "avi") {
            return "AVI (DivX/Xvid/MPEG-4)";
        }

        if (codecStr == "h264" || codecStr == "H264" || codecStr == "avc1" || codecStr == "AVC1")
            codecStrPrint = "H.264/AVC";
        else if (codecStr == "hev1" || codecStr == "HEVC" || codecStr == "hevc" ||
            codecStr == "hvc1" || codecStr == "h265" || codecStr == "H265")
            codecStrPrint = "H.265/HEVC";
        else if (codecStr == "MPG4" || codecStr == "mpg4" || codecStr == "mp4v" ||
            codecStr == "MP4V" || codecStr == "XVID" || codecStr == "DIVX")
            codecStrPrint = "MPEG-4";
        else if (codecStr == "VP80" || codecStr == "vp80")
            codecStrPrint = "VP8";
        else if (codecStr == "VP90" || codecStr == "vp90")
            codecStrPrint = "VP9";
        else if (codecStr == "AV01" || codecStr == "av01")
            codecStrPrint = "AV1";
        else if (codecStr == "MJPG" || codecStr == "mjpg")
            codecStrPrint = "Motion JPEG";
        else {
            std::string ext = VideoName.substr(VideoName.find_last_of(".") + 1);
            if (ext == "mp4" || ext == "m4v") codecStrPrint = "H.264/AVC (MP4)";
            if (ext == "avi") codecStrPrint = "AVI (DivX/Xvid/MPEG-4)";
            if (ext == "mkv") codecStrPrint = "MKV (different)";
            if (ext == "mov") codecStrPrint = "QuickTime (H.264/ProRes)";
            if (ext == "wmv") codecStrPrint = "Windows Media Video";
            if (ext == "flv") codecStrPrint = "Flash Video";
            if (ext == "webm") codecStrPrint = "WebM (VP8/VP9)";
        }
        return codecStrPrint;
    }
    else {
        std::string ConvertedFinalString = OldVideoName.substr(OldVideoName.find_last_of("\\/") + 1);
        std::string ext = OldVideoName.substr(OldVideoName.find_last_of(".") + 1);
        if (ext == "mp4" || ext == "m4v") codecStrPrint = "H.264/AVC (MP4)";
        if (ext == "avi") codecStrPrint = "AVI (DivX/Xvid/MPEG-4)";
        if (ext == "mkv") codecStrPrint = "MKV (разный)";
        if (ext == "mov") codecStrPrint = "QuickTime (H.264/ProRes)";
        if (ext == "wmv") codecStrPrint = "Windows Media Video";
        if (ext == "flv") codecStrPrint = "Flash Video";
        if (ext == "webm") codecStrPrint = "WebM (VP8/VP9)";
    }
}
//Функция получения размера видео
std::string GetSize(long long bytes) {
    char sizeText[32];
    if (bytes < 1024) {
        sprintf(sizeText, "Video size: %.2f B", (double)bytes);
    }
    else if (bytes > 1024 && bytes < 1024 * 1024) {
        double kb = bytes / 1024.0;
        sprintf(sizeText, "Video size: %.2f KB", kb);
    }
    else if (bytes >= 1024 * 1024 && bytes < 1024 * 1024 * 1024) {
        double mb = bytes / (1024.0 * 1024.0);
        sprintf(sizeText, "Video size: %.2f MB", mb);
    }
    else if (bytes >= 1024 * 1024 * 1024) {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        sprintf(sizeText, "Video size: %.2f GB", gb);
    }
    return std::string(sizeText);
}
//Функция отрисовки свойств
void FeaturesDraw(cv::Mat& resultWin, const cv::Rect& sizeOfWindow, std::string VideoName, std::string OldVideoName, std::string Minutes, std::string Seconds, int WindowWidth, int WindowHeight, double fps, std::string codecStr, std::string sizeText) {
    int NameIndex = VideoName.find_last_of(".");
    int InfoY = (sizeOfWindow.height) / 4;
    int InfoX = (sizeOfWindow.width) / 4 + 40;
    cv::putText(resultWin, "Features",
        cv::Point(InfoX, InfoY + 40),
        FONT, FONT_SIZE_BASIC,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Name: " + VideoName.substr(0, NameIndex),
        cv::Point(InfoX, InfoY + 85),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Duration: " + Minutes + ":" + Seconds,
        cv::Point(InfoX, InfoY + 120),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    if (converted == false) {
        cv::putText(resultWin, "Video format: " + VideoName.substr(NameIndex),
            cv::Point(InfoX, InfoY + 155),
            FONT, FONT_SIZE_SMALL,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
    else {
        cv::putText(resultWin, "Video format: " + OldVideoName.substr(OldVideoName.find_last_of(".")),
            cv::Point(InfoX, InfoY + 155),
            FONT, FONT_SIZE_SMALL,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
    std::string videoResolutionX = std::to_string(WindowWidth);
    std::string videoResolutionY = std::to_string(WindowHeight);
    cv::putText(resultWin, "Video resolution: " + videoResolutionX + "x" + videoResolutionY,
        cv::Point(InfoX, InfoY + 190),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    std::string videoFPSstring = std::to_string((int)fps);
    cv::putText(resultWin, "Frame rate: " + videoFPSstring,
        cv::Point(InfoX, InfoY + 225),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    int IndexOfDot = Name.find_last_of("\\/") + 1;
    cv::putText(resultWin, "File location: " + Name.substr(0, IndexOfDot),
        cv::Point(InfoX, InfoY + 330),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Codec: " + codecStr,
        cv::Point(InfoX, InfoY + 260),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, sizeText,
        cv::Point(InfoX, InfoY + 295),
        FONT, FONT_SIZE_SMALL, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Exit",
        cv::Point((sizeOfWindow.width) / 2 + 262, (InfoY * 3) - 35),
        FONT, FONT_SIZE_SMALL,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
}
//Функция отрисовки меню скорости
int DrawSpeedMenu(int targetDelay, const cv::Rect& sizeOfWindow, cv::Mat& resultWin) {
    const int SPEED_MENU_POS_X = sizeOfWindow.width / 2 + SPEED_MENU_X_OFFSET;
    const int SPEED_MENU_POS_Y = sizeOfWindow.height - SPEED_MENU_Y_OFFSET;
    DrawSpecificFigure SpeedMenu(SPEED_MENU_POS_X, SPEED_MENU_POS_Y - 150, SPEED_MENU_WIDTH, SPEED_MENU_HEIGHT);
    SpeedMenu.DrawRoundedRectangle(resultWin, cv::Scalar(0, 0, 0), SPEED_MENU_RADIUS);
    SpeedMenuItem speedItems[] = {
    {"2x",   targetDelay / 2,   2.0f, -10},
    {"1.5x", targetDelay / 1.5, 1.5f, -50},
    {"1x",   targetDelay,   1.0f, -90},
    {"0.5x", targetDelay * 2, 0.5f, -130}
    };
    for (int i = 0; i < 4; i++) {
        int itemY = SPEED_MENU_POS_Y + speedItems[i].yOffset;
        cv::putText(resultWin, speedItems[i].TEXT, cv::Point(SPEED_MENU_POS_X + (i == 1 || i == 3 ? 4.5 : 13.5), itemY),
            FONT, FONT_SIZE_SMALL, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        bool isHover;
        if (mousePos.x > SPEED_MENU_POS_X - 10 && mousePos.x < SPEED_MENU_POS_X + 60 &&
            mousePos.y > itemY - 35 && mousePos.y < itemY) {
            isHover = true;
        }
        else {
            isHover = false;
        }

        if (isHover) {
            cv::rectangle(resultWin,
                cv::Point(sizeOfWindow.width / 2 + 290 - (i == 0 || i == 2 ? 22.5 : 29), itemY + 7),
                cv::Point(sizeOfWindow.width / 2 + 290 + (i == 0 || i == 2 ? 2.5 : 12.5), itemY + 6),
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        if (mouseClicked && isHover) {
            targetDelay = speedItems[i].delay;
            Speed_Text = speedItems[i].TEXT;
            ma_sound_set_pitch(&audioSound, speedItems[i].speed);
        }
    }
    return targetDelay;
}
//Функция проверки на спящий режим
void SleepMode() {
        auto nowTimer = std::chrono::steady_clock::now();
        auto elapsedTimer = std::chrono::duration_cast<std::chrono::milliseconds>(nowTimer - startTimer).count();
        if (oldMousePos != mousePos || mouseClicked) {
            startTimer = nowTimer;
            IsSleep = false; 
        }
        
        oldMousePos = mousePos;
        if (elapsedTimer >= 10000) {
            IsSleep = true;
        }
        else {
            IsSleep = false;
            
        }
}
//Функция проверки горячих клавиш
void CheckButtonCodes(int key, cv::Mat& frame, cv::VideoCapture& cap) {
        if (key == 32) { // кнопка Space (пауза)
            isPaused = !isPaused;
            if (audioInitialized) {
                if (isPaused) ma_sound_stop(&audioSound);
                else ma_sound_start(&audioSound);
            }
        }
        else if (key == 114 || key == 82 || key == 234 || key == 202) { //кнопка R (перезагрузка)
           cap.set(cv::CAP_PROP_POS_FRAMES, 0);
           if (audioInitialized) ma_sound_seek_to_second(&audioSound, 0);
        }
        else if (key == 248 || key ==216 || key == 105 || key == 73) { //кнопка I (свойства)
            featuresActive = !featuresActive;
        }
        else if (key == 251 || key == 219 || key == 115 || key == 83) { //кнопка S (скриншот)
            GetScreen(frame);
        }
        else if (key == 228 || key == 196 || key == 76 || key == 108) { //кнопка L (скриншот)
            loopActive = !loopActive;
        }
        //Получаем коды нажатых кнопок в консоли
        if (key > 0) {
            std::cout << "Key code: " << key << std::endl;
        }
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Rus");

    char exePath[MAX_PATH]; // Сюда запишем путь к файлу
    GetModuleFileNameA(NULL, exePath, MAX_PATH); // получаем путь к нашему файлу, где NULL - это текущий файл, exePath - куда писать результат, MAX_PATH - размер записи
    std::string exeDir = exePath; // строка, в которой обрежем имя файла чтоб оставить только путь
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/")); // обрезаем строку с первого символа до последнего найденного слеша
    SetCurrentDirectoryA(exeDir.c_str()); // меняем рабочую папку на папку с программой

    //Открываем файл
    if (argc > 1) {
        //Файл передан через "Открыть с помощью"
        Name = argv[1];
        std::cout << "Открываю файл: " << Name << std::endl;
    }
    else {
        //Файл не передан — используем значение по умолчанию
        Name = "Mult.mp4";
        std::cout << "Использую файл по умолчанию: " << Name << std::endl;
    }
    //Передан файл с расширением .webm
    std::string OldName = Name;
    if (Name.find(".webm")  != std::string::npos) {
        Name = ConvertWEBMtoMP4(Name);
        converted = true;
    }

    // находим видео 
    cv::VideoCapture cap(Name);
    if (!cap.isOpened()) return -1;
    std::string command = "ffmpeg -i \"" + Name + "\" -vn -acodec libmp3lame audio.mp3 -y -loglevel quiet";
    cv::Mat frame, resized;
    bool firstFrame = true;
    cv::Rect windowSize;

    //создаем окно 
    cv::namedWindow("Video Player", cv::WINDOW_NORMAL);
    cv::setWindowProperty("Video Player", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    cv::Mat loadingScreen = cv::Mat::zeros(screenHeight, screenWidth, CV_8UC3);
   
    //Вывод надписи о загрузке
        for (int i = 0; i < 4; i++) {
            loadingScreen = cv::Mat::zeros(screenHeight, screenWidth, CV_8UC3);  // очищаем
            std::string LoadingString = "Loading";
            for (int j = 0; j < (i % 4); j++) LoadingString += ".";
                cv::putText(loadingScreen, LoadingString, cv::Point((screenWidth / 2) - 70, screenHeight / 2),
                FONT, 1.2, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                cv::imshow("Video Player", loadingScreen);
                cv::waitKey(300);  // пауза 200 мс
        }
    std::cout << "Извлекаю аудио..." << std::endl;
    system(command.c_str());

    // ИНИЦИАЛИЗАЦИЯ ЗВУКА
    if (!std::filesystem::exists("audio.mp3")) {
        std::cerr << "audio.mp3 не найден" << std::endl;
    }
    else if (ma_engine_init(NULL, &audioEngine) == MA_SUCCESS) {
        if (ma_sound_init_from_file(&audioEngine, "audio.mp3", 0, NULL, NULL, &audioSound) == MA_SUCCESS) {
            ma_sound_start(&audioSound);
            audioInitialized = true;
            std::cout << " Звук загружен!" << std::endl;
        }
        else {
            std::cerr << " Не удалось загрузить audio.mp3" << std::endl;
            ma_engine_uninit(&audioEngine);
        }
    }
    else {
        std::cerr << " Не удалось инициализировать звук" << std::endl;
    }

    //вычисления для ФПС и корректного времени воспроизведения
    int totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    double fps = cap.get(cv::CAP_PROP_FPS);
    int targetDelay = (int)(1000.0 / fps);
    int x2Delay = targetDelay / 2;
    int x1_5Delay = targetDelay / 1.5;
    int x1Delay = targetDelay;
    int x0_5Delay = targetDelay * 2;
    int framesIn10Seconds = (int)(fps * 10);
    std::cout << "Длина видео: " << targetDelay << "мс" << std::endl;
    std::cout << "ФПС: " << fps << std::endl;

    //Получение информации о видео
    float audioLength;
    ma_sound_get_length_in_seconds(&audioSound, &audioLength);
    double videoLength = totalFrames / fps;
    std::cout << "Длина видео: " << videoLength << " сек" << std::endl;
    std::cout << "Длина аудио: " << audioLength << " сек" << std::endl;
    //узнаем размеры видео
    int videoW = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int videoH = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Video: " << videoW << "x" << videoH << std::endl;
    cv::setMouseCallback("Video Player", onMouse);

    auto lastFrameTime = std::chrono::steady_clock::now();

    cap >> frame;
    if (frame.empty()) return -1;
    
    //Получаем кодек
    std::string codecStr = GetCodec(cap);
    //Получаем размер
    long long bytes = std::filesystem::file_size(Name);
    std::string sizeString = std::to_string(bytes);
    //Устанавливаем иконку
    SetIcon("Video Player","Icon.ico");

    while (true) {
        if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) <= 0) {
            std::cout << "Окно закрыто" << std::endl;
            break;
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
            (currentTime - lastFrameTime).count();
        //Проверка на окончание видео
        if (!isPaused && elapsed >= targetDelay) {
            cv::Mat newFrame;
            cap >> newFrame;
            if (newFrame.empty()) {
                frame.release();
            }
            else {
                frame = newFrame;
                lastFrameTime = currentTime;
            }
        }
        if (frame.empty()) {
            if (loopActive) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (audioInitialized) {
                        ma_sound_stop(&audioSound);           // сначала останавливаем
                        ma_sound_seek_to_second(&audioSound, 0);  // перематываем в начало
                        if (!isPaused) ma_sound_start(&audioSound);  // запускаем если не на паузе
                }
                continue;
            }
            else {
                if (audioInitialized) {
                    ma_sound_stop(&audioSound);
                }
                cv::destroyAllWindows();
            }
            break;
        }

        if (firstFrame) {
            windowSize = cv::getWindowImageRect("Video Player");
            if (windowSize.width <= 0 || windowSize.height <= 0) {
                windowSize = cv::Rect(0, 0, 1280, 720);
            }
            std::cout << "Размеры окна " << windowSize.width << "x" << windowSize.height << std::endl;
            firstFrame = false;
        }
        cv::Mat result = cv::Mat::zeros(windowSize.height, windowSize.width, frame.type());
        int NewY = windowSize.height - 80;
        if (NewY <= 0) {
            NewY = 100;
        }
        float windowAspect = (float)windowSize.height / windowSize.width;
        float videoAspect = (float)videoH / videoW;

        if (windowAspect < videoAspect) {
            int newHeight = NewY;
            int newWidth = (int)(newHeight / videoAspect);
            int offsetX = std::max(0, (windowSize.width - newWidth) / 2);
            cv::resize(frame, resized, cv::Size(newWidth, newHeight));
            cv::Rect roi(offsetX, 0, newWidth, newHeight);
            resized.copyTo(result(roi));
        }
        else {
            int newWidth = windowSize.width;
            int newHeight = (int)(newWidth * videoAspect);
            int offsetY = 0;
            cv::resize(frame, resized, cv::Size(newWidth, newHeight));

            cv::Rect roi(0, offsetY, newWidth, newHeight);
            resized.copyTo(result(roi));
        }

        int currentFrame = cap.get(cv::CAP_PROP_POS_FRAMES);
        int currentTimeMinutes = (currentFrame / fps) / 60;
        int currentTimeSeconds = (int)(currentFrame / fps) % 60;
        int totalTimeMinutes = (totalFrames / fps) / 60;
        int totalTimeSeconds = (int)(totalFrames / fps) % 60;
        int remainingTime = (totalFrames / fps) - (currentFrame / fps);
        DrawInterface Interface(windowSize.width / 2, windowSize.height - 55, cap);
        //Название видео
        std::string FinalName;
        Interface.Name(result, windowSize, Name, OldName,  FinalName);

        if (featuresActive == true) {
            DrawSpecificFigure FeaturesBG((windowSize.width) / 4, (windowSize.height) / 4, (windowSize.width) / 2, (windowSize.height) / 2);
            FeaturesBG.DrawRoundedRectangle(result, cv::Scalar(30, 30, 30), 20);
            std::string sizeText = GetSize(bytes);
            std::string codecStrPrint = SetCodec(codecStr,FinalName,OldName);
            if (mousePos.x > (windowSize.width) / 2 + 215 && mousePos.x < (windowSize.width) / 2 + 230 + 115 && mousePos.y < (((windowSize.height) / 4) * 3) - 15 && mousePos.y >(((windowSize.height) / 4) * 3) - 60) {
                DrawSpecificFigure FeaturesBGExit((windowSize.width) / 2 + 230, (((windowSize.height) / 4) * 3) - 50, 100, 20);
                FeaturesBGExit.DrawRoundedRectangle(result, cv::Scalar(50, 50, 50), 10);
            }
            if (mouseClicked && (mousePos.x > (windowSize.width) / 2 + 215 && mousePos.x < (windowSize.width) / 2 + 230 + 115 && mousePos.y < (((windowSize.height) / 4) * 3) - 15 && mousePos.y >(((windowSize.height) / 4) * 3) - 60)) {
                mouseClicked = !mouseClicked;
                featuresActive = false;
            }
            FeaturesDraw(result, windowSize, FinalName, OldName, std::to_string(totalTimeMinutes), std::to_string(totalTimeSeconds), videoW, videoH, fps, codecStrPrint, sizeText);
        }

        //Кнопка скриншота
        if (IsSleep == false) {
            //
            Interface.DrawProgressBar(result, windowSize, currentFrame, totalFrames, fps, cap);
            //Кнопка свойств видео
            Interface.InfoButton(result, windowSize);
            //Кнопка скриншота
            Interface.ScreenshotButton(result, windowSize, frame);
            // Кнопка возврата в начало
            Interface.ReloadButton(0, result, totalFrames, fps);
            //Кнопка перемотки в конец
            Interface.ReloadButton(1, result, totalFrames, fps);
            // Кнопка перемотки назад
            Interface.RewindButton(0, result, currentFrame, framesIn10Seconds, fps);
            //Кнопка перемотки вперед
            Interface.RewindButton(1, result, currentFrame, framesIn10Seconds, fps);
            // Кнопка паузы
            Interface.PauseButton(result, isPaused);
            // Кнопка скорости
            Interface.SpeedButton(windowSize, result);
            if (SpeedMenuActive) {
                DrawSpeedMenu(targetDelay, windowSize, result);
                targetDelay = DrawSpeedMenu(targetDelay, windowSize, result);
            }
            int volumeBarX = windowSize.width - VOLUME_BAR_X_OFFSET;
            int volumeBarY = windowSize.height - VOLUME_BAR_Y_OFFSET;
            // Отрисовка шкалы громкости
            Interface.VolumeBar(windowSize, result, volumeBarX, volumeBarY);
            //Отрисовка кнопки громкости
            Interface.VolumeButton(result, volumeBarX, volumeBarY);
            //Отрисовка кнопки повтора
            Interface.LoopButton(result, windowSize);
        }
        //Отрисовка времени
        Interface.Time(result, windowSize, remainingTime, currentTimeMinutes, currentTimeSeconds, totalTimeMinutes, totalTimeSeconds);
        //Отрисовка индикатора воспроизведения
        Interface.DrawBouncingBar(windowSize, result, isPaused);

        ma_sound_set_volume(&audioSound, volume);
        cv::imshow("Video Player", result);

        SleepMode();

        if (mouseClicked) {
            SpeedMenuActive = false;
            mouseClicked = false;
        }

        cv::imshow("Video Player", result);
        int key = cv::waitKey(1);
        if (key == 27) break; //  кнопка ESC (выход)
        CheckButtonCodes(key, frame, cap);
    }

    if (audioInitialized) {
        audioInitialized = false;
        ma_sound_uninit(&audioSound);
        ma_engine_uninit(&audioEngine);
    }
    cv::destroyAllWindows();
    cap.release();
    if (converted == true ) {
        remove(Name.c_str());
    }
    remove("audio.mp3");
    return 0;
}
