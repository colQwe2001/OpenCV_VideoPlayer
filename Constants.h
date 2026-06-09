#pragma once
#include <opencv2/opencv.hpp>

namespace UI {
	// Цвета
	const cv::Scalar UI_COLOR(192, 192, 192);
	// Шрифты
	const int FONT = cv::FONT_HERSHEY_DUPLEX;
	const double FONT_SIZE_SMALL = 0.6;
	const double FONT_SIZE_TIME = 0.7;
	const double FONT_SIZE_MEDIUM = 0.8;
	const double FONT_SIZE_BASIC = 1.0;
	// Размеры кнопок
	const int BTN_RADIUS_SMALL = 15;
	const int BTN_RADIUS_BASIC = 20;
	const int BTN_RADIUS_PAUSE = 25;
	// Позиции кнопок относительно центра
	const int BTN_INFO_OFFSET = 370;
	const int BTN_SCREENSHOT_OFFSET = 285;
	const int BTN_RELOAD_OFFSET = 210;
	const int BTN_REWIND_OFFSET = 110;
	const int BTN_SPEED_OFFSET = 280;
	// Позиции кнопок относительно края
	const int BTN_LOOP_OFFSET = 420;
	const int BOUSING_BAR_OFFSET = 175;
	// Шкала прогресса
	const int PROGRESS_Y_OFFSET = 84;
	const int PROGRESS_X = 50;
	const int PROGRESS_WIDTH_OFFSET = 100;
	const int PROGRESS_HEIGHT = 6;
	const int HANDLE_RADIUS = 5;
	// Время
	const int TIME_OFFSET_X = 150;
	const int TIME_OFFSET_Y = 40;
	// Шкала громкости
	const int VOLUME_BAR_X_OFFSET = 320;
	const int VOLUME_BAR_Y_OFFSET = 48;
	const int VOLUME_BAR_WIDTH = 100;
	const int VOLUME_BAR_HEIGHT = 5;
	//Меню скорости
	const int SPEED_MENU_X_OFFSET = 255;
	const int SPEED_MENU_Y_OFFSET = 80;
	const int SPEED_MENU_WIDTH = 50;
	const int SPEED_MENU_HEIGHT = 150;
	const int SPEED_MENU_RADIUS = 11;
	// Имя файла
	/*const int MAX_NAME_LEN = 10;
	const int CUT_NAME_LEN = 13;*/

}