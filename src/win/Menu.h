//
// Created by Xenia on 1.09.2024.
//

#ifndef MENU_H
#define MENU_H
#include "UI.h"



class Menu {
public:
    static void DrawUI();
    static void Setup();
    static WNDCLASSEX wc;
    static HWND hwnd;
};



#endif //MENU_H
