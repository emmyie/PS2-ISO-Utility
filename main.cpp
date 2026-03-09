#include "ui/menu.hpp"

int WinMain( )
{
	auto ctx = menu::init_menu_ctx( );

	while ( !glfwWindowShouldClose( ctx.m_window ) )
	{
		glfwPollEvents( );
		menu::begin_frame( );
		menu::render_ui( );
		menu::end_frame( ctx );
	}

	menu::shutdown( ctx );
	return 0;
}
