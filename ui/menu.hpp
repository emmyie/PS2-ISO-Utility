#pragma once
#include <GLFW/glfw3.h>
#include <functional>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <thread>

#include "../utility/iso_splitter.hpp"
#include "file_browser.hpp"
#include "theme.hpp"

#include "../utility/image_loader.hpp"
#include <GL/gl.h>
#include <cstdint>
#include <exception>
#include <string>

namespace menu
{
	struct context
	{
		GLFWwindow* m_window = nullptr;
	};

	inline file_browser g_file_browser;

	inline context init_menu_ctx( int width = 1280, int height = 720, const char* title = "ISO Splitter" )
	{
		if ( !glfwInit( ) )
		{
			throw std::runtime_error( "Failed to init GLFW" );
		}

		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );

		context ctx;
		ctx.m_window = glfwCreateWindow( width, height, title, nullptr, nullptr );
		if ( !ctx.m_window )
			throw std::runtime_error( "Failed to create window" );

		glfwMakeContextCurrent( ctx.m_window );
		glfwSwapInterval( 1 );

		IMGUI_CHECKVERSION( );
		ImGui::CreateContext( );
		ImGuiIO& io = ImGui::GetIO( ); ( void )io;
		ImGui::StyleColorsDark( );

		theme::load_fonts( );

		theme::apply_theme( );

		ImGui_ImplGlfw_InitForOpenGL( ctx.m_window, true );
		ImGui_ImplOpenGL3_Init( "#version 130" );

		return ctx;
	}

	inline void shutdown( context& ctx )
	{
		ImGui_ImplOpenGL3_Shutdown( );
		ImGui_ImplGlfw_Shutdown( );
		ImGui::DestroyContext( );
		if ( ctx.m_window )
		{
			glfwDestroyWindow( ctx.m_window );
			ctx.m_window = nullptr;
		}
		glfwTerminate( );
	}

	inline void begin_frame( )
	{
		ImGui_ImplOpenGL3_NewFrame( );
		ImGui_ImplGlfw_NewFrame( );
		ImGui::NewFrame( );
	}

	inline void do_background( )
	{
		static GLuint background_image = 0;

		if ( background_image == 0 )
		{
			background_image = load_texture_from_file( "images/background.png" );
		}

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoBackground;

		ImGuiViewport* viewport = ImGui::GetMainViewport( );
		ImGui::SetNextWindowPos( viewport->Pos );
		ImGui::SetNextWindowSize( viewport->Size );

		theme::begin_window( "##background", NULL, flags );

		if ( background_image != 0 )
		{
			ImGui::GetWindowDrawList( )->AddImage(
				background_image,
				viewport->Pos,
				ImVec2( viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y )
			);
		}

		theme::end_window( );
	}

	inline void render_ui( )
	{
		do_background( );

		theme::begin_window( "ISO Splitter" );

		static std::string result_string;
		static float progress = 0.0f;
		static bool splitting = false;
		static char name_buffer[ 256 ] = { 0 };

		if ( theme::button( "Select ISO" ) )
		{
			g_file_browser.open( );
		}

		g_file_browser.draw( "File Browser" );

		if ( !g_file_browser.m_selected_file.empty( ) )
		{
			theme::text( "%s", g_file_browser.m_selected_file.c_str( ) );
		}

		ImGui::InputText( "##name", name_buffer, IM_ARRAYSIZE( name_buffer ) );

		ImGui::SameLine( );

		if ( theme::button( "Split ISO" ) )
		{
			if ( !g_file_browser.m_selected_file.empty( ) )
			{
				std::string title = name_buffer;

				split_options opt;
				opt.m_title = title;
				opt.m_on_progress_fn = [ & ] ( uint64_t processed, uint64_t total )
				{
					progress = static_cast< float >( processed ) / static_cast< float >( total );
				};

				splitting = true;
				result_string.clear( );

				std::string iso_path = g_file_browser.m_selected_file;
				std::string out_dir = iso_path + ".split";

				std::thread( [ iso_path, out_dir, opt ] ( )
				{
					try
					{
						auto result = iso_splitter::split( iso_path, out_dir, opt );
						if ( !result.m_written_files.empty( ) )
						{
							result_string = "Success!";
						}
						else
						{
							result_string = "Failed or dry run!";
						}
					}
					catch ( const std::exception& e )
					{
						result_string = std::string( "Error: " ) + e.what( );
					}
					splitting = false;
					progress = 0.0f;
				} ).detach( );
			}
			else
			{
				result_string = "No ISO selected!";
			}
		}

		if ( splitting )
		{
			theme::progress_bar( progress, ImVec2( 200, 20 ), "Splitting..." );
		}

		if ( !result_string.empty( ) )
		{
			theme::text( "Result: %s", result_string.c_str( ) );
		}

		theme::end_window( );
	}

	inline void end_frame( context& ctx )
	{
		ImGui::Render( );
		int display_w, display_h;
		glfwGetFramebufferSize( ctx.m_window, &display_w, &display_h );
		glViewport( 0, 0, display_w, display_h );
		glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );

		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData( ) );

		glfwSwapBuffers( ctx.m_window );
	}
}
