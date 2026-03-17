#include <iron.h>
#include <iron_wren.h>

int kickstart(int argc, char **argv) {
	(void)argc;
	(void)argv;
	
	wren_init();
	wren_eval("Iron.print(\"Hello from Wren!\")");
	wren_eval("Iron.print(\"Math test:\")");
	wren_eval("var x = Math.sin(0)");
	wren_eval("Iron.print(\"sin(0) = %(x)\")");
	wren_free();
	
	return 0;
}
