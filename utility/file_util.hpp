#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace file_util
{
	inline void read_exact( std::ifstream& f, char* dst, size_t n )
	{
		f.read( dst, static_cast< std::streamsize >( n ) );
		if ( static_cast< size_t >( f.gcount( ) ) != n )
		{
			throw std::runtime_error( "Short read from file" );
		}
	}

	inline std::vector<char> read_block( const std::filesystem::path& path, size_t size, std::streamoff offset = 0 )
	{
		std::ifstream f{ path, std::ios::binary };
		if ( !f )
		{
			throw std::runtime_error( "Cannot open file: " + path.string( ) );
		}

		if ( offset > 0 )
		{
			f.seekg( offset, std::ios::beg );
		}

		std::vector<char> buf( size );
		read_exact( f, buf.data( ), size );
		return buf;
	}
};
