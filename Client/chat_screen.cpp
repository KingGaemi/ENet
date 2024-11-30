#include "chat_screen.hpp"

ChatScreen::ChatScreen(){}

ChatScreen::~ChatScreen()
{
	endwin();
	delwin(inputwin);
}

void ChatScreen::Init()
{
	// ncurses setup
	initscr();

	// create input box
	int yMax, xMax;
	getmaxyx(stdscr, yMax, xMax);
	inputwin = newwin(3, xMax-12, yMax-5, 5);
	box(inputwin, 0, 0);
	refresh();
	wrefresh(inputwin);
}

std::string ChatScreen::CheckBoxInput()
{
	// Clear the input box
	wclear(inputwin);
	box(inputwin, 0, 0);

	// Await user input
	char msg[80];
	mvwscanw(inputwin, 1, 1, "%[^\n]", msg);

	return msg;
}

void ChatScreen::ShowChatMessage(const char* username,const char* msg)
{
	mvprintw(msg_y, 1, "%s: %s", username, msg);
	refresh();
	msg_y++;
}