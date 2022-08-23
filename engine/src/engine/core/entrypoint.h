#ifndef ENGINE_CORE_ENTRYPOINT_H
#define ENGINE_CORE_ENTRYPOINT_H

#include "application.h"

extern application* create_app();

int main(const int argc, const char** argv) {
	application* app = create_app();

	if (!app->create()) {
		app->terminate();
		return -1;
	}

	bool success = app->start();

	app->terminate();
	delete app;
	return success ? 0 : -1;
}

#endif //ENGINE_CORE_ENTRYPOINT_H