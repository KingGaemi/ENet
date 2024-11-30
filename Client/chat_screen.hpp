#ifndef CHAT_SCREEN_HPP
#define CHAT_SCREEN_HPP

#include <stdio.h>

#include <iostream>
#include <string>
#include <ncursesw/ncurses.h>


class ChatScreen {
public:
	ChatScreen();
	~ChatScreen();

	void Init();

	void ShowChatMessage(const char *username, const char *msg);

	std::string CheckBoxInput();


private:
	int msg_y = 0;
	WINDOW * inputwin = nullptr;

};

#endif