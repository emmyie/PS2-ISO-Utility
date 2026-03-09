#pragma once
#include <cstdarg>
#include <imgui.h>
#include <imgui_internal.h>

class theme
{
public:
	static void load_fonts( )
	{
		ImGuiIO& io = ImGui::GetIO( );
		io.Fonts->AddFontDefault( );
		io.Fonts->Build( );
	}

	static void apply_theme( )
	{
		ImGuiStyle& style = ImGui::GetStyle( );

		ImVec4 accent_pink = ImVec4( 0.95f, 0.70f, 0.85f, 1.00f ); // A light, pastel pink
		ImVec4 soft_purple = ImVec4( 0.85f, 0.75f, 0.95f, 1.00f ); // A soft, pastel purple
		ImVec4 light_gray = ImVec4( 0.90f, 0.90f, 0.90f, 1.00f );  // Light, neutral background
		ImVec4 dark_gray = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );   // Darker text/lines
		ImVec4 window_bg = ImVec4( 0.98f, 0.95f, 0.99f, 0.94f );   // Very light, translucent window background

		style.Colors[ ImGuiCol_Text ] = dark_gray;
		style.Colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.60f, 0.60f, 0.60f, 1.00f );
		style.Colors[ ImGuiCol_WindowBg ] = window_bg;
		style.Colors[ ImGuiCol_PopupBg ] = light_gray;
		style.Colors[ ImGuiCol_Border ] = soft_purple;
		style.Colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
		style.Colors[ ImGuiCol_FrameBg ] = light_gray;
		style.Colors[ ImGuiCol_FrameBgHovered ] = soft_purple;
		style.Colors[ ImGuiCol_FrameBgActive ] = accent_pink;
		style.Colors[ ImGuiCol_TitleBg ] = soft_purple;
		style.Colors[ ImGuiCol_TitleBgActive ] = accent_pink;
		style.Colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 0.90f, 0.85f, 0.95f, 0.50f );
		style.Colors[ ImGuiCol_MenuBarBg ] = light_gray;
		style.Colors[ ImGuiCol_ScrollbarBg ] = light_gray;
		style.Colors[ ImGuiCol_ScrollbarGrab ] = soft_purple;
		style.Colors[ ImGuiCol_ScrollbarGrabHovered ] = accent_pink;
		style.Colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 1.00f, 0.80f, 0.90f, 1.00f );
		style.Colors[ ImGuiCol_CheckMark ] = accent_pink;
		style.Colors[ ImGuiCol_SliderGrab ] = soft_purple;
		style.Colors[ ImGuiCol_SliderGrabActive ] = accent_pink;
		style.Colors[ ImGuiCol_Button ] = soft_purple;
		style.Colors[ ImGuiCol_ButtonHovered ] = ImVec4( 1.00f, 0.80f, 0.90f, 1.00f );
		style.Colors[ ImGuiCol_ButtonActive ] = accent_pink;
		style.Colors[ ImGuiCol_Header ] = soft_purple;
		style.Colors[ ImGuiCol_HeaderHovered ] = accent_pink;
		style.Colors[ ImGuiCol_HeaderActive ] = ImVec4( 1.00f, 0.80f, 0.90f, 1.00f );
		style.Colors[ ImGuiCol_Separator ] = dark_gray;
		style.Colors[ ImGuiCol_SeparatorHovered ] = accent_pink;
		style.Colors[ ImGuiCol_SeparatorActive ] = soft_purple;
		style.Colors[ ImGuiCol_ResizeGrip ] = soft_purple;
		style.Colors[ ImGuiCol_ResizeGripHovered ] = accent_pink;
		style.Colors[ ImGuiCol_ResizeGripActive ] = ImVec4( 1.00f, 0.80f, 0.90f, 1.00f );
		style.Colors[ ImGuiCol_Tab ] = ImVec4( 0.90f, 0.85f, 0.95f, 0.86f );
		style.Colors[ ImGuiCol_TabHovered ] = soft_purple;
		style.Colors[ ImGuiCol_TabActive ] = accent_pink;
		style.Colors[ ImGuiCol_TabUnfocused ] = ImVec4( 0.90f, 0.85f, 0.95f, 0.52f );
		style.Colors[ ImGuiCol_TabUnfocusedActive ] = ImVec4( 0.95f, 0.70f, 0.85f, 0.80f );
		style.Colors[ ImGuiCol_PlotLines ] = accent_pink;
		style.Colors[ ImGuiCol_PlotLinesHovered ] = soft_purple;
		style.Colors[ ImGuiCol_PlotHistogram ] = soft_purple;
		style.Colors[ ImGuiCol_PlotHistogramHovered ] = accent_pink;
		style.Colors[ ImGuiCol_TextSelectedBg ] = ImVec4( 0.95f, 0.70f, 0.85f, 0.35f );
		style.Colors[ ImGuiCol_DragDropTarget ] = ImVec4( 0.95f, 0.70f, 0.85f, 1.00f );
		style.Colors[ ImGuiCol_NavHighlight ] = soft_purple;
		style.Colors[ ImGuiCol_NavWindowingHighlight ] = soft_purple;
		style.Colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0.20f, 0.20f, 0.20f, 0.20f );
		style.Colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.20f, 0.20f, 0.20f, 0.35f );

		style.WindowRounding = 10.0f;
		style.FrameRounding = 8.0f;
		style.PopupRounding = 8.0f;
		style.GrabRounding = 8.0f;
		style.ChildRounding = 8.0f;
		style.TabRounding = 8.0f;
		style.ScrollbarRounding = 12.0f;
		style.GrabMinSize = 20.0f;
	}

	static bool begin_window( const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0 )
	{
		bool result = ImGui::Begin( name, p_open, flags );

		return result;
	}

	static void end_window( )
	{
		ImGui::End( );
	}

	static bool button( const char* label, const ImVec2& size = ImVec2( 0, 0 ) )
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if ( window->SkipItems )
			return false;

		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID( label );
		const ImVec2 label_size = ImGui::CalcTextSize( label, NULL, true );

		ImVec2 button_size = size;
		if ( button_size.x == 0 ) button_size.x = label_size.x + style.FramePadding.x * 2.0f;
		if ( button_size.y == 0 ) button_size.y = label_size.y + style.FramePadding.y * 2.0f;

		const ImVec2 pos = window->DC.CursorPos;
		const ImRect bb( pos, ImVec2( pos.x + button_size.x, pos.y + button_size.y ) );
		ImGui::ItemSize( bb, style.FramePadding.y );
		if ( !ImGui::ItemAdd( bb, id ) )
		{
			return false;
		}

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );

		ImU32 col = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );

		ImVec2 shadow_offset( 2.0f, 2.0f );
		ImGui::GetWindowDrawList( )->AddRectFilled( ImVec2( bb.Min.x + shadow_offset.x, bb.Min.y + shadow_offset.y ), ImVec2( bb.Max.x + shadow_offset.x, bb.Max.y + shadow_offset.y ), IM_COL32( 0, 0, 0, 40 ), style.FrameRounding );

		ImGui::GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, col, style.FrameRounding );

		ImGui::RenderTextClipped( ImVec2( bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y ), ImVec2( bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y ), label, NULL, &label_size, style.ButtonTextAlign, &bb );

		return pressed;
	}

	static void progress_bar( float fraction, const ImVec2& size_arg = ImVec2( -1, 0 ), const char* overlay = NULL )
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow( );
		if ( window->SkipItems )
		{
			return;
		}

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		ImVec2 size = size_arg;
		if ( size.x < 0 )
		{
			size.x = ImGui::GetContentRegionAvail( ).x;
		}

		const ImVec2 pos = window->DC.CursorPos;
		const ImRect bb( pos, ImVec2( pos.x + size.x, pos.y + size.y ) );
		ImGui::ItemSize( size, style.FramePadding.y );
		if ( !ImGui::ItemAdd( bb, 0 ) )
		{
			return;
		}

		ImVec2 shadow_offset( 1.0f, 1.0f );
		ImGui::GetWindowDrawList( )->AddRectFilled( ImVec2( bb.Min.x + shadow_offset.x, bb.Min.y + shadow_offset.y ), ImVec2( bb.Max.x + shadow_offset.x, bb.Max.y + shadow_offset.y ), IM_COL32( 0, 0, 0, 20 ), style.FrameRounding );

		ImU32 bg_col = ImGui::GetColorU32( ImGuiCol_FrameBg );
		ImGui::GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, bg_col, style.FrameRounding );

		if ( fraction > 0.0f )
		{
			ImU32 fill_col = ImGui::GetColorU32( ImGuiCol_PlotHistogram );
			ImVec2 fill_size( bb.Min.x + ( bb.Max.x - bb.Min.x ) * ImSaturate( fraction ), bb.Max.y );
			ImGui::GetWindowDrawList( )->AddRectFilled( bb.Min, fill_size, fill_col, style.FrameRounding );
		}

		if ( overlay )
		{
			ImVec2 overlay_size = ImGui::CalcTextSize( overlay, NULL );
			if ( overlay_size.x > 0.0f )
				ImGui::RenderTextClipped( ImVec2( bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y ), ImVec2( bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y ), overlay, NULL, &overlay_size, style.ButtonTextAlign, &bb );
		}
	}

	static void text( const char* fmt, ... ) IM_FMTARGS( 2 )
	{
		va_list args;
		va_start( args, fmt );
		ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle( ).Colors[ ImGuiCol_Text ] );
		ImGui::TextV( fmt, args );
		ImGui::PopStyleColor( );
		va_end( args );
	}
};