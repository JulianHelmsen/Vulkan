#include <stdlib.h>
#include <engine/engine.h>
#include <stdio.h>


int main(const int argc, const char** argv) {
	

	window window{"Title", 1020, 720};

	if (!render_api::init(window)) {
		window.destroy();
		return -1;
	}


	while (!window.is_closed_requsted()) {
		window.wait_events();
	}

	render_api::shutdown();
	window.destroy();
	return 0;
}