#include "image_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint load_texture_from_file( const char* filename )
{
	GLuint tx_id = 0;
	glGenTextures( 1, &tx_id );
	if ( tx_id == 0 )
	{
		return 0;
	}

	glBindTexture( GL_TEXTURE_2D, tx_id );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	int width = 0, height = 0, channels = 0;
	unsigned char* data = stbi_load( filename, &width, &height, &channels, 0 );
	if ( data )
	{
		GLenum format = GL_RGBA;
		if ( channels == 1 )
			format = GL_RED;
		else if ( channels == 3 )
			format = GL_RGB;
		else if ( channels == 4 )
			format = GL_RGBA;

		glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data );

		stbi_image_free( data );
	}
	else
	{
		glDeleteTextures( 1, &tx_id );
		return 0;
	}

	return tx_id;
}
