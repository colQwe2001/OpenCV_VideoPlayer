#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm> 
#include <cstdlib>
#include <chrono>
#include <windows.h>
#include <filesystem>

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
const cv::Scalar UI_COLOR(192, 192, 192);
int fontFace = cv::FONT_HERSHEY_DUPLEX;
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

void InfoDraw(cv::Mat& resultWin, cv::Rect& sizeOfWindow) {
    int btnX = sizeOfWindow.width / 2 - 370;
    int btnY = sizeOfWindow.height - 37.5;
    cv::putText(resultWin, "i",
        cv::Point(btnX, btnY),
        fontFace, 0.8,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    float dist = std::hypot(mousePos.x - (btnX + 5), mousePos.y - (btnY - 7));
    if (mouseClicked && dist <= 15) {
        featuresActive = !featuresActive;
        mouseClicked = false;
    }
    else if (dist <= 15) {
        cv::circle(resultWin, cv::Point(btnX + 3, btnY - 8), 15, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
}
std::string GetCodec(std::string codecStr, std::string VideoName, std::string OldVideoName) {
    std::string codecStrPrint;
    if (converted == false) {
        if (codecStr == "h264" || codecStr == "H264" || codecStr == "avc1" || codecStr == "AVC1") codecStrPrint = "H.264/AVC";
        else if (codecStr == "hev1" || codecStr == "HEVC" || codecStr == "hevc" || codecStr == "hvc1" || codecStr == "h265" || codecStr == "H265") codecStrPrint = "H.265/HEVC";
        else if (codecStr == "h261" || codecStr == "H261") codecStrPrint = "H.261";
        else if (codecStr == "h262" || codecStr == "H262") codecStrPrint = "H.261/MPEG-2";
        else if (codecStr == "h263" || codecStr == "H263") codecStrPrint = "H.263";
        else if (codecStr == "MPG1" || codecStr == "mpg1" || codecStr == "MPEG") codecStrPrint = "MPEG - 1";
        else if (codecStr == "MPG2" || codecStr == "mpg2") codecStrPrint = "MPEG - 2";
        else if (codecStr == "MPG4" || codecStr == "mpg4" || codecStr == "mp4v" || codecStr == "MP4V" || codecStr == "XVID" || codecStr == "DIVX") codecStrPrint = "MPEG-4";
        else if (codecStr == "WMV1" || codecStr == "wmv1") codecStrPrint = "Windows Media Video 7";
        else if (codecStr == "WMV2" || codecStr == "wmv2") codecStrPrint = "Windows Media Video 8";
        else if (codecStr == "WMV3" || codecStr == "wmv3") codecStrPrint = "Windows Media Video 9";
        else if (codecStr == "RV10" || codecStr == "rv10") codecStrPrint = "RealVideo 1.0";
        else if (codecStr == "RV20" || codecStr == "rv20") codecStrPrint = "RealVideo 2.0";
        else if (codecStr == "RV30" || codecStr == "rv30") codecStrPrint = "RealVideo 3.0";
        else if (codecStr == "VP80" || codecStr == "vp80") codecStrPrint = "VP8";
        else if (codecStr == "VP90" || codecStr == "vp90") codecStrPrint = "VP9";
        else if (codecStr == "AV01" || codecStr == "av01") codecStrPrint = "AV1";
        else if (codecStr == "JPEG" || codecStr == "jpeg" || codecStr == "mjpa" || codecStr == "mjpb" || codecStr == "MJPG" || codecStr == "mjpg") codecStrPrint = "Motion JPEG";
        else if (codecStr == "PNG" || codecStr == "png") codecStrPrint = "PNG";
        else if (codecStr == "apch" || codecStr == "apcn" || codecStr == "apco" || codecStr == "ap4h") codecStrPrint = "Apple ProRes";
        else if (codecStr == "DV25" || codecStr == "dv25") codecStrPrint = "DV (MiniDV)";
        else if (codecStr == "DV50" || codecStr == "dv50") codecStrPrint = "DVCPRO 50";
        else if (codecStr == "dvc") codecStrPrint = "DVCPRO HD";
        else if (codecStr == "MJPEG" || codecStr == "mjpeg" || codecStr == "MJPG") codecStrPrint = "MJPEG";
        else if (codecStr == "FLV1" || codecStr == "flv1") codecStrPrint = "Flash Video";
        else if (codecStr == "THEO" || codecStr == "theo") codecStrPrint = "Theora";
        else if (codecStr == "LAGS" || codecStr == "lags") codecStrPrint = "Lagarith";
        else if (codecStr == "HFYU" || codecStr == "hfy u") codecStrPrint = "HuffYUV";
        else if (codecStr == "FFV1" || codecStr == "ffv1") codecStrPrint = "FFV1";
        else if (codecStr == "MAGY" || codecStr == "magy") codecStrPrint = "MagicYUV";
        else if (codecStr == "UTVI" || codecStr == "utvi") codecStrPrint = "Ut Video";
        else if (codecStr == "CVID" || codecStr == "cvid") codecStrPrint = "Cinepak";
        else if (codecStr == "IV32" || codecStr == "iv32") codecStrPrint = "Intel Indeo 3.2";
        else if (codecStr == "IV41" || codecStr == "iv41") codecStrPrint = "Intel Indeo 4.1";
        else if (codecStr == "IV50" || codecStr == "iv50") codecStrPrint = "Intel Indeo 5.0";
        else if (codecStr == "QTRP" || codecStr == "qtrp") codecStrPrint = "QuickTime RPZA";
        else if (codecStr == "SMC" || codecStr == "smc") codecStrPrint = "QuickTime SMC";
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

void FeaturesDraw(cv::Mat& resultWin, cv::Rect& sizeOfWindow, std::string VideoName, std::string OldVideoName, std::string Minutes, std::string Seconds, int WindowWidth, int WindowHeight, double fps, std::string codecStr, std::string sizeText) {
    int NameIndex = VideoName.find_last_of(".");
    int InfoY = (sizeOfWindow.height) / 4;
    int InfoX = (sizeOfWindow.width) / 4 + 40;
    cv::putText(resultWin, "Features",
        cv::Point(InfoX, InfoY + 40),
        fontFace, 1.0,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Name: " + VideoName.substr(0, NameIndex),
        cv::Point(InfoX, InfoY + 85),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Duration: " + Minutes + ":" + Seconds,
        cv::Point(InfoX, InfoY + 120),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    if (converted == false) {
        cv::putText(resultWin, "Video format: " + VideoName.substr(NameIndex),
            cv::Point(InfoX, InfoY + 155),
            fontFace, 0.6,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
    else {
        cv::putText(resultWin, "Video format: " + OldVideoName.substr(OldVideoName.find_last_of(".")),
            cv::Point(InfoX, InfoY + 155),
            fontFace, 0.6,
            cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    }
    std::string videoResolutionX = std::to_string(WindowWidth);
    std::string videoResolutionY = std::to_string(WindowHeight);
    cv::putText(resultWin, "Video resolution: " + videoResolutionX + "x" + videoResolutionY,
        cv::Point(InfoX, InfoY + 190),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    std::string videoFPSstring = std::to_string((int)fps);
    cv::putText(resultWin, "Frame rate: " + videoFPSstring,
        cv::Point(InfoX, InfoY + 225),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    int IndexOfDot = Name.find_last_of("\\/") + 1;
    cv::putText(resultWin, "File location: " + Name.substr(0, IndexOfDot),
        cv::Point(InfoX, InfoY + 330),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Codec: " + codecStr,
        cv::Point(InfoX, InfoY + 260),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, sizeText,
        cv::Point(InfoX, InfoY + 295),
        fontFace, 0.6, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
    cv::putText(resultWin, "Exit",
        cv::Point((sizeOfWindow.width) / 2 + 262, (InfoY * 3) - 35),
        fontFace, 0.6,
        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
}

void DrawSceenshotButton(cv::Mat& resultWin, cv::Rect& sizeOfWindow, cv::Mat& frame) {
    int buttonXCamera = sizeOfWindow.width / 2 - 285;
    int buttonYCamera = sizeOfWindow.height - 40;
    int dxCamera = mousePos.x - (buttonXCamera + 5);
    int dyCamera = mousePos.y - (buttonYCamera - 7);
    int camX = buttonXCamera;
    int camY = buttonYCamera;

    // Корпус
    DrawSpecificFigure Camera(buttonXCamera - 10, buttonYCamera - 10, 20, 10);
    Camera.DrawRoundedRectangle(resultWin, UI_COLOR, 2);
    DrawSpecificFigure CameraUp(buttonXCamera - 3, buttonYCamera - 13, 6, 3);
    CameraUp.DrawRoundedRectangle(resultWin, UI_COLOR, 1);

    // Объектив
    cv::circle(resultWin, cv::Point(buttonXCamera, buttonYCamera - 5), 5,
        cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

    //Вспышка
    cv::rectangle(resultWin,
        cv::Point(buttonXCamera + 7, buttonYCamera - 8),
        cv::Point(buttonXCamera + 10, buttonYCamera - 10),
        cv::Scalar(0, 0, 0), -1, cv::LINE_AA);

    float distanceCamera = std::sqrt(dxCamera * dxCamera + dyCamera * dyCamera);

    if (mouseClicked && distanceCamera <= 20) {
        GetScreen(frame);
        mouseClicked = false;
    }
    else if (distanceCamera <= 20) {
        cv::circle(resultWin, cv::Point(buttonXCamera, sizeOfWindow.height - 45), 20, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
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
                fontFace, 1.2, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
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
            if (audioInitialized) {
                ma_sound_stop(&audioSound);
            }
            cv::destroyAllWindows();
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
        std::string totalTimeStringMinutes = std::to_string(totalTimeMinutes);
        std::string totalTimeStringSeconds = std::to_string(totalTimeSeconds);
        std::string currentTimeStringMinutes = std::to_string(currentTimeMinutes);
        std::string currentTimeStringSeconds = std::to_string(currentTimeSeconds);
        int remainingTime = (totalFrames / fps) - (currentFrame / fps);
        int remainingTimeMinutes = remainingTime / 60;
        int remainingTimeSeconds = remainingTime % 60;
        std::string remainingTimeMinutesString = std::to_string(remainingTimeMinutes);
        std::string remainingTimeSecondsString = std::to_string(remainingTimeSeconds);

        //Название видео
        std::string FinalName;
        if (converted == false) {
           FinalName = Name.substr(Name.find_last_of("\\/") + 1);
        }
        else {
           FinalName = OldName.substr(OldName.find_last_of("\\/") + 1);
        }
        if (IsSleep == false) {
            if (FinalName.length() < 10) {
                cv::putText(result, FinalName,cv::Point(50, windowSize.height - 37.5),fontFace, 0.9,cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
            else {
                cv::putText(result, FinalName.substr(0, 13) + ".." + FinalName.substr(FinalName.find_last_of(".")),
                cv::Point(50, windowSize.height - 37.5),fontFace, 0.9,cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
        }
     
        //Кнопка свойств видео
        if (IsSleep == false) {
            InfoDraw(result, windowSize);
        }
        int NameIndex = FinalName.find_last_of(".");
        if (featuresActive == true) {
            DrawSpecificFigure FeaturesBG((windowSize.width) / 4, (windowSize.height) / 4, (windowSize.width) / 2, (windowSize.height) / 2);
            FeaturesBG.DrawRoundedRectangle(result, cv::Scalar(30, 30, 30), 20);
            std::string sizeText = GetSize(bytes);
            std::string codecStrPrint = GetCodec(codecStr,FinalName,OldName);
            if (mousePos.x > (windowSize.width) / 2 + 215 && mousePos.x < (windowSize.width) / 2 + 230 + 115 && mousePos.y < (((windowSize.height) / 4) * 3) - 15 && mousePos.y >(((windowSize.height) / 4) * 3) - 60) {
                DrawSpecificFigure FeaturesBGExit((windowSize.width) / 2 + 230, (((windowSize.height) / 4) * 3) - 50, 100, 20);
                FeaturesBGExit.DrawRoundedRectangle(result, cv::Scalar(50, 50, 50), 10);
            }
            if (mouseClicked && (mousePos.x > (windowSize.width) / 2 + 215 && mousePos.x < (windowSize.width) / 2 + 230 + 115 && mousePos.y < (((windowSize.height) / 4) * 3) - 15 && mousePos.y >(((windowSize.height) / 4) * 3) - 60)) {
                mouseClicked = !mouseClicked;
                featuresActive = false;
            }
            FeaturesDraw(result, windowSize, FinalName, OldName, totalTimeStringMinutes, totalTimeStringSeconds, videoW, videoH, fps, codecStrPrint, sizeText);
        }

        //Кнопка скриншота
        if (IsSleep == false) {
         DrawSceenshotButton(result, windowSize, frame);  
        }
        
       
        if (IsSleep == false) {
            //Отрисовка шкалы прогресса
            int barY = windowSize.height - 84;
            int barX = 50;
            int barWidth = windowSize.width - 100;
            int barHeight = 6;
            cv::rectangle(result,
                cv::Point(barX, barY - barHeight / 2),
                cv::Point(barX + barWidth, barY + barHeight / 2),
                cv::Scalar(60, 60, 60), -1);
            int progressWidth = (int)((currentFrame / (double)totalFrames) * barWidth);
            cv::rectangle(result,
                cv::Point(barX, barY - barHeight / 2),
                cv::Point(barX + progressWidth, barY + barHeight / 2),
                cv::Scalar(UI_COLOR), -1);
            int circlePosX = (int)progressWidth + barX;
            cv::circle(result, cv::Point(circlePosX, barY), 5, cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            if (mouseClicked &&
                mousePos.y > barY - barHeight - 5 && mousePos.y < barY + 5 &&
                mousePos.x > barX && mousePos.x < barX + barWidth && SpeedMenuActive == false) {
                float clickPercent = (mousePos.x - barX) / (float)barWidth;
                int newFrame = (int)(totalFrames * clickPercent);
                cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
                if (audioInitialized) {
                    ma_sound_seek_to_second(&audioSound, newFrame / fps);
                }
                mouseClicked = false;
            }
        }

        // ОТРИСОВКА ИНТЕРФЕЙСА
        if (IsSleep == false) {
            // Кнопка возврата в начало
            int buttonXReload = windowSize.width / 2 - 210;
            int buttonYReload = windowSize.height - 55;
            int dxr = mousePos.x - (buttonXReload + 10);
            int dyr = mousePos.y - (buttonYReload + 10);
            float distanceR = std::sqrt(dxr * dxr + dyr * dyr);

            if (mouseClicked && distanceR <= 20) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (audioInitialized) ma_sound_seek_to_second(&audioSound, 0);
                mouseClicked = false;
            }
            else if (distanceR <= 20) {
                cv::circle(result, cv::Point(windowSize.width / 2 - 200, windowSize.height - 45), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            cv::rectangle(result,
                cv::Point(buttonXReload + 1, buttonYReload + 2.5),
                cv::Point(buttonXReload + 4, buttonYReload + 17.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);

            cv::Point triangleR[3];
            triangleR[0] = cv::Point(buttonXReload + 18, buttonYReload + 2.5);
            triangleR[1] = cv::Point(buttonXReload + 18, buttonYReload + 17.5);
            triangleR[2] = cv::Point(buttonXReload + 6, buttonYReload + 10);
            cv::fillConvexPoly(result, triangleR, 3,
                cv::Scalar(UI_COLOR), cv::LINE_AA);

            // Кнопка перемотки назад
            int buttonXBack = windowSize.width / 2 - 110;
            int buttonYBack = windowSize.height - 55;
            int dxBack = mousePos.x - (buttonXBack + 10);
            int dyBack = mousePos.y - (buttonYBack + 10);
            float distanceBack = std::sqrt(dxBack * dxBack + dyBack * dyBack);

            if (mouseClicked && distanceBack <= 20) {
                int newFrame = std::max(currentFrame - framesIn10Seconds, 0);
                cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
                if (audioInitialized) {
                    double newTime = newFrame / fps;
                    ma_sound_seek_to_second(&audioSound, newTime);
                }
                mouseClicked = false;
            }
            else if (distanceBack <= 20) {
                cv::circle(result, cv::Point(windowSize.width / 2 - 100, windowSize.height - 45), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            cv::Point triangleBack[3];
            triangleBack[0] = cv::Point(buttonXBack + 20, buttonYBack + 2.5);
            triangleBack[1] = cv::Point(buttonXBack + 20, buttonYBack + 17.5);
            triangleBack[2] = cv::Point(buttonXBack + 8, buttonYBack + 10);
            cv::fillConvexPoly(result, triangleBack, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);

            cv::Point triangleBack2[3];
            triangleBack2[0] = cv::Point(buttonXBack + 8, buttonYBack + 2.5);
            triangleBack2[1] = cv::Point(buttonXBack + 8, buttonYBack + 17.5);
            triangleBack2[2] = cv::Point(buttonXBack - 4, buttonYBack + 10);
            cv::fillConvexPoly(result, triangleBack2, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);

            // Кнопка паузы
            int buttonX = windowSize.width / 2 - 15;
            int buttonY = windowSize.height - 60;
            int ButtonCenterX = buttonX + 15;
            int ButtonCenterY = buttonY + 15;
            int dx = mousePos.x - ButtonCenterX;
            int dy = mousePos.y - ButtonCenterY;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (mouseClicked && distance <= 25) {
                isPaused = !isPaused;
                if (audioInitialized) {
                    if (isPaused) ma_sound_stop(&audioSound);
                    else ma_sound_start(&audioSound);
                }
                mouseClicked = false;
            }
            else if (distance <= 25) {
                cv::circle(result, cv::Point(windowSize.width / 2, windowSize.height - 45), 25,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            if (isPaused) {
                cv::Point pts[3];
                pts[0] = cv::Point(buttonX + 10, buttonY + 7.5);
                pts[1] = cv::Point(buttonX + 10, buttonY + 22.5);
                pts[2] = cv::Point(buttonX + 25, buttonY + 15);
                cv::fillConvexPoly(result, pts, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);
            }
            else {
                cv::rectangle(result,
                    cv::Point(buttonX + 7.5, buttonY + 7.5),
                    cv::Point(buttonX + 12.5, buttonY + 22.5),
                    cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
                cv::rectangle(result,
                    cv::Point(buttonX + 17.5, buttonY + 7.5),
                    cv::Point(buttonX + 22.5, buttonY + 22.5),
                    cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            }

            // Кнопка перемотки вперед
            int buttonXFor = windowSize.width / 2 + 110;
            int buttonYFor = windowSize.height - 55;
            int dxFor = mousePos.x - (buttonXFor - 10);
            int dyFor = mousePos.y - (buttonYFor + 10);
            float distanceFor = std::sqrt(dxFor * dxFor + dyFor * dyFor);

            if (mouseClicked && distanceFor <= 20) {
                int newFrame = std::min(currentFrame + framesIn10Seconds, totalFrames);
                cap.set(cv::CAP_PROP_POS_FRAMES, newFrame);
                if (audioInitialized) {
                    double newTime = newFrame / fps;
                    ma_sound_seek_to_second(&audioSound, newTime);
                }
                mouseClicked = false;
            }
            else if (distanceFor <= 20) {
                cv::circle(result, cv::Point(windowSize.width / 2 + 100, windowSize.height - 45), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            cv::Point triangleFor[3];
            triangleFor[0] = cv::Point(buttonXFor - 8, buttonYFor + 2.5);
            triangleFor[1] = cv::Point(buttonXFor - 8, buttonYFor + 17.5);
            triangleFor[2] = cv::Point(buttonXFor + 4, buttonYFor + 10);
            cv::fillConvexPoly(result, triangleFor, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);

            cv::Point triangleFor2[3];
            triangleFor2[0] = cv::Point(buttonXFor - 20, buttonYFor + 2.5);
            triangleFor2[1] = cv::Point(buttonXFor - 20, buttonYFor + 17.5);
            triangleFor2[2] = cv::Point(buttonXFor - 8, buttonYFor + 10);
            cv::fillConvexPoly(result, triangleFor2, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);

            // Кнопка перемотки в конец
            int buttonXEnd = windowSize.width / 2 + 210;
            int buttonYEnd = windowSize.height - 55;
            int dxEnd = mousePos.x - (buttonXEnd - 10);
            int dyEnd = mousePos.y - (buttonYEnd + 10);
            float distanceEnd = std::sqrt(dxEnd * dxEnd + dyEnd * dyEnd);

            if (mouseClicked && distanceEnd <= 20) {
                cap.set(cv::CAP_PROP_POS_FRAMES, totalFrames - 1);
                if (audioInitialized) ma_sound_seek_to_second(&audioSound, totalFrames / fps);
                mouseClicked = false;
            }
            else if (distanceEnd <= 20) {
                cv::circle(result, cv::Point(windowSize.width / 2 + 200, windowSize.height - 45), 20,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            cv::rectangle(result,
                cv::Point(buttonXEnd - 4, buttonYEnd + 2.5),
                cv::Point(buttonXEnd - 1, buttonYEnd + 17.5),
                cv::Scalar(UI_COLOR), -1, cv::LINE_AA);

            cv::Point triangleE[3];
            triangleE[0] = cv::Point(buttonXEnd - 18, buttonYEnd + 2.5);
            triangleE[1] = cv::Point(buttonXEnd - 18, buttonYEnd + 17.5);
            triangleE[2] = cv::Point(buttonXEnd - 6, buttonYEnd + 10);
            cv::fillConvexPoly(result, triangleE, 3, cv::Scalar(UI_COLOR), cv::LINE_AA);



            // Кнопка скорости
            int buttonXSpeed = windowSize.width / 2 + 300;
            int buttonYSpeed = windowSize.height - 55;
            int ButtonCenterXSpeed = buttonXSpeed - 10;
            int ButtonCenterYSpeed = buttonYSpeed + 10;

            int dxSpeed = mousePos.x - ButtonCenterXSpeed;
            int dySpeed = mousePos.y - ButtonCenterYSpeed;
            float distanceSpeed = std::sqrt(dxSpeed * dxSpeed + dySpeed * dySpeed);

            int XspeedPos = windowSize.width / 2 + 300 - 35;
            int YspeedPos = windowSize.height - 45 - 35;

            if (mouseClicked && distanceSpeed <= 20) {
                mouseClicked = false;
                SpeedMenuActive = !SpeedMenuActive;
            }
            if (distanceSpeed <= 20) {
                cv::rectangle(result,
                    cv::Point(windowSize.width / 2 + 289 - line_width, windowSize.height - 45 + 11.5),
                    cv::Point(windowSize.width / 2 + 289 + line_width, windowSize.height - 45 + 12),
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }

            cv::putText(result, Speed_Text,
                cv::Point(ButtonCenterXSpeed - Speed_Text_X, ButtonCenterYSpeed + 6),
                fontFace, 0.6,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);

            if (SpeedMenuActive) {
                int x1 = windowSize.width / 2 + 300 - 35;
                int x2 = windowSize.width / 2 + 300 + 35;
                int y1 = YspeedPos;
                int y2 = YspeedPos - 150;

                cv::ellipse(result, cv::Point(x1 + 15, y2),
                    cv::Size(15, 15), 180, 0, 90, cv::Scalar(0, 0, 0), -1, cv::LINE_AA);
                cv::ellipse(result, cv::Point(x2 - 15, y2),
                    cv::Size(15, 15), 270, 0, 90, cv::Scalar(0, 0, 0), -1, cv::LINE_AA);
                cv::rectangle(result,
                    cv::Point(windowSize.width / 2 + 300 - 20, YspeedPos - 150),
                    cv::Point(windowSize.width / 2 + 300 + 20, YspeedPos - 165),
                    cv::Scalar(0, 0, 0), -1);
                cv::rectangle(result,
                    cv::Point(windowSize.width / 2 + 300 - 35, YspeedPos - 150),
                    cv::Point(windowSize.width / 2 + 300 + 35, YspeedPos),
                    cv::Scalar(0, 0, 0), -1);

                cv::putText(result, "2x", cv::Point(XspeedPos + 21, YspeedPos - 10),
                    fontFace, 0.6, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                if (mouseClicked && mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 35 && mousePos.y < YspeedPos) {
                    targetDelay = x2Delay;
                    line_width = 12.5;
                    Speed_Text_X = 14;
                    Speed_Text = "2x";
                    ma_sound_set_pitch(&audioSound, 2.0f);
                    SpeedMenuActive = false;
                    mouseClicked = false;
                }
                if (mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 35 && mousePos.y < YspeedPos) {
                    cv::rectangle(result,
                        cv::Point(windowSize.width / 2 + 300 - 12.5, YspeedPos - 4),
                        cv::Point(windowSize.width / 2 + 300 + 12.5, YspeedPos - 5),
                        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                }
                cv::putText(result, "1.5x", cv::Point(XspeedPos + 12, YspeedPos - 50),
                    fontFace, 0.6, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                if (mouseClicked && mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 70 && mousePos.y < YspeedPos - 35) {
                    targetDelay = x1_5Delay;
                    line_width = 22.5;
                    Speed_Text_X = 24;
                    Speed_Text = "1.5x";
                    ma_sound_set_pitch(&audioSound, 1.5f);
                    SpeedMenuActive = false;
                    mouseClicked = false;
                }
                if (mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 70 && mousePos.y < YspeedPos - 35) {
                    cv::rectangle(result,
                        cv::Point(windowSize.width / 2 + 300 - 20, YspeedPos - 44),
                        cv::Point(windowSize.width / 2 + 300 + 22.5, YspeedPos - 45),
                        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                }
                cv::putText(result, "1x", cv::Point(XspeedPos + 21, YspeedPos - 90),
                    fontFace, 0.6, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                if (mouseClicked && mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 105 && mousePos.y < YspeedPos - 70) {
                    targetDelay = x1Delay;
                    line_width = 12.5;
                    Speed_Text_X = 14;
                    Speed_Text = "1x";
                    ma_sound_set_pitch(&audioSound, 1.0f);
                    SpeedMenuActive = false;
                    mouseClicked = false;
                }
                if (mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 105 && mousePos.y < YspeedPos - 70) {
                    cv::rectangle(result,
                        cv::Point(windowSize.width / 2 + 300 - 12.5, YspeedPos - 84),
                        cv::Point(windowSize.width / 2 + 300 + 12.5, YspeedPos - 85),
                        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                }
                cv::putText(result, "0.5x", cv::Point(XspeedPos + 12, YspeedPos - 130),
                    fontFace, 0.6, cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                if (mouseClicked && mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 160 && mousePos.y < YspeedPos - 105) {
                    targetDelay = x0_5Delay;
                    line_width = 22.5;
                    Speed_Text_X = 24;
                    Speed_Text = "0.5x";
                    ma_sound_set_pitch(&audioSound, 0.5f);
                    SpeedMenuActive = false;
                    mouseClicked = false;
                }
                if (mouseClicked && mousePos.x < XspeedPos && mousePos.x > XspeedPos + 70 &&
                    mousePos.y < YspeedPos - 160 && mousePos.y > YspeedPos) {
                    SpeedMenuActive = false;
                    mouseClicked = false;
                }
                if (mousePos.x > XspeedPos && mousePos.x < XspeedPos + 70 &&
                    mousePos.y > YspeedPos - 160 && mousePos.y < YspeedPos - 105) {
                    cv::rectangle(result,
                        cv::Point(windowSize.width / 2 + 300 - 20, YspeedPos - 124),
                        cv::Point(windowSize.width / 2 + 300 + 22.5, YspeedPos - 125),
                        cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                }
            }
        }
        // ОТРИСОВКА ШКАЛЫ ГРОМКОСТИ
        if (IsSleep == false) {
            int volumeBarX = windowSize.width - 350;
            int volumeBarY = windowSize.height - 48;
            int volumeBarWidth = 100;
            int volumeBarHeight = 5;
            int progressVolume = (int)(volumeBarWidth * volume);
            cv::rectangle(result, cv::Point(volumeBarX, volumeBarY), cv::Point(volumeBarX + volumeBarWidth, volumeBarY + volumeBarHeight), cv::Scalar(60, 60, 60), -1, cv::LINE_AA);
            cv::rectangle(result, cv::Point(volumeBarX, volumeBarY), cv::Point(volumeBarX + progressVolume, volumeBarY + volumeBarHeight), cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            if (mouseClicked &&
                mousePos.x >= volumeBarX && mousePos.x <= volumeBarX + volumeBarWidth &&
                mousePos.y >= volumeBarY && mousePos.y <= volumeBarY + volumeBarHeight) {

                float clickPercent = (mousePos.x - volumeBarX) / (float)volumeBarWidth;
                volume = std::max(0.0f, std::min(1.0f, clickPercent));
                mouseClicked = false;
            }
            cv::rectangle(result, cv::Point(volumeBarX - 37.5, volumeBarY - 2.5), cv::Point(volumeBarX - 30, volumeBarY + 7.5), cv::Scalar(UI_COLOR), -1, cv::LINE_AA);
            cv::Point pts[3] = {
        cv::Point(volumeBarX - 35, volumeBarY + 2.5),
        cv::Point(volumeBarX - 25, volumeBarY + 12.5),
        cv::Point(volumeBarX - 25, volumeBarY - 7.5)
            };
            cv::fillConvexPoly(result, pts, 3, cv::Scalar(UI_COLOR));
            if (volume > 0.8)
                cv::ellipse(result, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                    cv::Size(10, 12), 0, 270, 450,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            if (volume > 0.4)
                cv::ellipse(result, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                    cv::Size(6, 8), 0, 270, 450,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            if (volume > 0)
                cv::ellipse(result, cv::Point(volumeBarX - 20, volumeBarY + 2.5),
                    cv::Size(2, 4), 0, 270, 450,
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            if (volume < 0.01f) {
                cv::line(result,
                    cv::Point(volumeBarX - 20, volumeBarY - 2.5),
                    cv::Point(volumeBarX - 10, volumeBarY + 7.5),
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
                cv::line(result,
                    cv::Point(volumeBarX - 20, volumeBarY + 7.5),
                    cv::Point(volumeBarX - 10, volumeBarY - 2.5),
                    cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
            }
            if (mouseClicked &&
                mousePos.x > volumeBarX - 40 && mousePos.x < volumeBarX - 20 &&
                mousePos.y > volumeBarY - 10 && mousePos.y < volumeBarY + 10 && is_silent == false) {
                save_volume = volume;
                volume = 0.0f;
                mouseClicked = false;
                is_silent = true;
            }
            if (mouseClicked &&
                mousePos.x > volumeBarX - 40 && mousePos.x < volumeBarX - 20 &&
                mousePos.y > volumeBarY - 10 && mousePos.y < volumeBarY + 10 && is_silent == true) {
                volume = save_volume;
                mouseClicked = false;
                is_silent = false;
            }
        }
        //Время видео
        int TimeCenterX = windowSize.width - 150;
        int TimeCenterY = windowSize.height - 40;
        if (mouseClicked && (mousePos.x > TimeCenterX+0 && mousePos.x < TimeCenterX + TimeStringLenght) && (mousePos.y > TimeCenterY - 10 && mousePos.y < TimeCenterY + 10)) {
            CurrentTime = !CurrentTime;
            mouseClicked = false;
        }
        if (CurrentTime == true) {
            cv::putText(result, currentTimeStringMinutes + ":" + currentTimeStringSeconds + "/" + totalTimeStringMinutes + ":" + totalTimeStringSeconds,
                cv::Point( TimeCenterX, windowSize.height - 40),
                fontFace, 0.7,
                cv::Scalar(UI_COLOR), 1, cv::LINE_AA);
        }
        else {
            cv::putText(result, "-" + remainingTimeMinutesString + ":" + remainingTimeSecondsString + "/" + totalTimeStringMinutes + ":" + totalTimeStringSeconds,
                cv::Point(TimeCenterX, windowSize.height - 40),
                fontFace, 0.7,
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
        if (CurrentTime == false && remainingTimeMinutes >= 10) {
            TimeStringLenght = 90;
        }
        if (CurrentTime == false && remainingTimeSeconds >= 10) {
            TimeStringLenght = 90;
        }
        if (CurrentTime == false && remainingTimeSeconds >= 10 && remainingTimeMinutes >= 10) {
            TimeStringLenght = 100;
        }
        ma_sound_set_volume(&audioSound, volume);

        cv::imshow("Video Player", result);

        //Спящий режим
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

        if (mouseClicked) {
            SpeedMenuActive = false;
            mouseClicked = false;
        }

        cv::imshow("Video Player", result);
        int key = cv::waitKey(1);
        if (key == 27) break; //  кнопка ESC (выход)
        else if (key == 32) { // кнопка Space (пауза)
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
        //Получаем коды нажатых кнопок в консоли
        /*if (key > 0) {
            std::cout << "Key code: " << key << std::endl;
        }*/
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
