#include <stdlib.h>
#include <engine/core/window.h>
#include <stdio.h>


int main(const int argc, const char** argv) {
	

	window window{"Title", 1020, 720};

	while (!window.is_closed()) {
		window.wait_events();
	}

	window.destroy();
	return 0;
}