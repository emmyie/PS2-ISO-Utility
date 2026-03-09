#pragma once
#include <filesystem>
#include <imgui.h>
#include <string>

#include "theme.hpp"

namespace fs = std::filesystem;

struct file_browser
{
	fs::path m_cur_path = fs::current_path( );
	std::string m_selected_file;
	bool m_open = false;

	void open( )
	{
		m_open = true;
	}

	void draw( const char* title )
	{
		if ( !m_open )
		{
			return;
		}

		theme::begin_window( title, &m_open );

		if ( m_cur_path.has_parent_path( ) && ImGui::Button( "<-" ) )
		{
			m_cur_path = m_cur_path.parent_path( );
		}

		ImGui::Separator( );

		for ( auto& entry : fs::directory_iterator( m_cur_path ) )
		{
			if ( entry.is_directory( ) )
			{
				if ( ImGui::Selectable( ( "[DIR] " + entry.path( ).filename( ).string( ) ).c_str( ) ) )
				{
					m_cur_path = entry.path( );
				}
			}
		}

		for ( auto& entry : fs::directory_iterator( m_cur_path ) )
		{
			if ( entry.is_regular_file( ) && entry.path( ).extension( ) == ".iso" )
			{
				bool is_selected = ( m_selected_file == entry.path( ).string( ) );

				if ( ImGui::Selectable( entry.path( ).filename( ).string( ).c_str( ), is_selected ) )
				{
					m_selected_file = entry.path( ).string( );
				}

				if ( ImGui::IsItemHovered( ) && ImGui::IsMouseDoubleClicked( 0 ) )
				{
					m_selected_file = entry.path( ).string( );
					m_open = false;
				}
			}
		}

		ImGui::Separator( );

		if ( theme::button( "OK" ) && !m_selected_file.empty( ) )
		{
			m_open = false;
		}

		ImGui::SameLine( );

		if ( theme::button( "Cancel" ) )
		{
			m_selected_file.clear( );
			m_open = false;
		}

		theme::end_window( );
	}

	bool is_open( ) const
	{
		return m_open;
	}
};
