#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "../utility/iso_util.hpp"

using namespace std;
using namespace iso_util;

static void write_sector( std::ofstream& f, uint64_t sector, const vector<char>& data )
{
	f.seekp( sector * sector_size, ios::beg );
	f.write( data.data( ), data.size( ) );
}

static void write_sector_fill( std::ofstream& f, uint64_t sector, size_t size )
{
	f.seekp( sector * sector_size, ios::beg );
	vector<char> buf( size, 0 );
	f.write( buf.data( ), buf.size( ) );
}

void test_extract_boot_id( )
{
	{
		string sys = "BOOT2 = cdrom0:\\\\SLES_123.45;1\n";
		string id = iso_util::extract_boot_id( sys );
		assert( id == "SLES_123.45" );
	}
	{
		string sys = "boot = CDROM0:\\FOO.ELF;1\r\n";
		string id = iso_util::extract_boot_id( sys );
		assert( id == "FOO.ELF" );
	}
	{
		string sys = "SOME=THING\n CDROM0:\\PATH\\TO\\GAME.ELF;1\n";
		string id = iso_util::extract_boot_id( sys );
		assert( id == "GAME.ELF" );
	}
	{
		string sys = "BOOT = \"CDROM0:\\QUOTED.ELF;1\"\n";
		string id = iso_util::extract_boot_id( sys );
		assert( id == "QUOTED.ELF" );
	}
	cout << "extract_boot_id tests passed\n";
}

void test_read_system_cnf_minimal( )
{
	const filesystem::path iso_path = "test_iso_minimal.iso";
	// Build a minimal ISO with PVD at sector 16 and root dir at LBA 20
	const uint32_t root_lba = 20;
	const uint32_t root_size = 1 * sector_size; // one sector
	const uint32_t file_lba = 30;

	// Content of SYSTEM.CNF
	const string syscnf = "BOOT = cdrom0:\\\\GAME_ID;1\n";
	vector<char> sysbuf( syscnf.begin( ), syscnf.end( ) );

	// create file and ensure it's large enough
	{
		ofstream f( iso_path, ios::binary | ios::trunc );
		assert( f );

		// ensure file has enough size by writing zeros up to last sector we'll use
		uint64_t last_sector = file_lba + 8; // a few sectors beyond
		f.seekp( ( last_sector + 1 ) * sector_size - 1, ios::beg );
		char z = 0;
		f.write( &z, 1 );

		// Prepare PVD sector
		vector<char> pvd( sector_size, 0 );
		pvd[ 0 ] = 1; // PVD
		memcpy( pvd.data( ) + 1, "CD001", 5 );
		pvd[ 6 ] = 1; // version

		const size_t RDR_OFF = 0x9c;
		// root dir record area (34 bytes) - we'll construct fields expected by iso_util
		vector<uint8_t> rdr( 34, 0 );
		// at offset 2 (in rdr) put root LBA little endian
		rdr[ 2 ] = static_cast< uint8_t >( root_lba & 0xFF );
		rdr[ 3 ] = static_cast< uint8_t >( ( root_lba >> 8 ) & 0xFF );
		rdr[ 4 ] = static_cast< uint8_t >( ( root_lba >> 16 ) & 0xFF );
		rdr[ 5 ] = static_cast< uint8_t >( ( root_lba >> 24 ) & 0xFF );
		// at offset 10 put root size little endian
		rdr[ 10 ] = static_cast< uint8_t >( root_size & 0xFF );
		rdr[ 11 ] = static_cast< uint8_t >( ( root_size >> 8 ) & 0xFF );
		rdr[ 12 ] = static_cast< uint8_t >( ( root_size >> 16 ) & 0xFF );
		rdr[ 13 ] = static_cast< uint8_t >( ( root_size >> 24 ) & 0xFF );

		memcpy( pvd.data( ) + RDR_OFF, rdr.data( ), rdr.size( ) );

		// write PVD to sector 16
		write_sector( f, 16, pvd );

		// Build root directory sector with one directory record for SYSTEM.CNF;1
		vector<char> dir( sector_size, 0 );
		string ident = "SYSTEM.CNF;1";
		uint8_t id_len = static_cast< uint8_t >( ident.size( ) );
		uint8_t rec_len = static_cast< uint8_t >( 33 + id_len );
		dir[ 0 ] = rec_len;
		dir[ 1 ] = 0; // ext attr len
		// file LBA for SYSTEM.CNF (point to file_lba)
		uint32_t file_lba_le = file_lba;
		dir[ 2 ] = static_cast< char >( file_lba_le & 0xFF );
		dir[ 3 ] = static_cast< char >( ( file_lba_le >> 8 ) & 0xFF );
		dir[ 4 ] = static_cast< char >( ( file_lba_le >> 16 ) & 0xFF );
		dir[ 5 ] = static_cast< char >( ( file_lba_le >> 24 ) & 0xFF );
		// file size at offset 10 (we'll set later)
		uint32_t file_size = static_cast< uint32_t >( sysbuf.size( ) );
		dir[ 10 ] = static_cast< char >( file_size & 0xFF );
		dir[ 11 ] = static_cast< char >( ( file_size >> 8 ) & 0xFF );
		dir[ 12 ] = static_cast< char >( ( file_size >> 16 ) & 0xFF );
		dir[ 13 ] = static_cast< char >( ( file_size >> 24 ) & 0xFF );

		// file identifier length at offset 32
		dir[ 32 ] = id_len;
		memcpy( dir.data( ) + 33, ident.data( ), id_len );

		// write root dir sector at root_lba
		write_sector( f, root_lba, dir );

		// write the SYSTEM.CNF file data at file_lba
		vector<char> filedata = sysbuf;
		// pad to sector size
		filedata.resize( ( ( filedata.size( ) + sector_size - 1 ) / sector_size ) * sector_size, 0 );
		write_sector( f, file_lba, filedata );

		f.flush( );
		f.close( );
	}

	// Now call read_system_cnf and verify contents
	string got = iso_util::read_system_cnf( iso_path );
	// Our read returns raw bytes; convert to string and check it contains expected substring
	assert( got.find( "GAME_ID" ) != string::npos );

	// cleanup
	filesystem::remove( iso_path );

	cout << "read_system_cnf minimal test passed\n";
}

void test_missing_system_cnf( )
{
	const filesystem::path iso_path = "test_iso_missing.iso";
	// Create an ISO with a valid PVD/root but no SYSTEM.CNF entry
	const uint32_t root_lba = 20;
	const uint32_t root_size = 1 * sector_size;

	{
		ofstream f( iso_path, ios::binary | ios::trunc );
		assert( f );
		uint64_t last_sector = root_lba + 4;
		f.seekp( ( last_sector + 1 ) * sector_size - 1, ios::beg );
		char z = 0;
		f.write( &z, 1 );

		vector<char> pvd( sector_size, 0 );
		pvd[ 0 ] = 1;
		memcpy( pvd.data( ) + 1, "CD001", 5 );
		pvd[ 6 ] = 1;

		const size_t RDR_OFF = 0x9c;
		vector<uint8_t> rdr( 34, 0 );
		rdr[ 2 ] = static_cast< uint8_t >( root_lba & 0xFF );
		rdr[ 3 ] = static_cast< uint8_t >( ( root_lba >> 8 ) & 0xFF );
		rdr[ 4 ] = static_cast< uint8_t >( ( root_lba >> 16 ) & 0xFF );
		rdr[ 5 ] = static_cast< uint8_t >( ( root_lba >> 24 ) & 0xFF );
		rdr[ 10 ] = static_cast< uint8_t >( root_size & 0xFF );
		rdr[ 11 ] = static_cast< uint8_t >( ( root_size >> 8 ) & 0xFF );
		rdr[ 12 ] = static_cast< uint8_t >( ( root_size >> 16 ) & 0xFF );
		rdr[ 13 ] = static_cast< uint8_t >( ( root_size >> 24 ) & 0xFF );
		memcpy( pvd.data( ) + RDR_OFF, rdr.data( ), rdr.size( ) );
		write_sector( f, 16, pvd );

		// root dir with an unrelated file
		vector<char> dir( sector_size, 0 );
		string ident = "NOT_SYSTEM.TXT;1";
		uint8_t id_len = static_cast< uint8_t >( ident.size( ) );
		uint8_t rec_len = static_cast< uint8_t >( 33 + id_len );
		dir[ 0 ] = rec_len;
		dir[ 32 ] = id_len;
		memcpy( dir.data( ) + 33, ident.data( ), id_len );
		write_sector( f, root_lba, dir );

		f.flush( );
		f.close( );
	}

	bool threw = false;
	try
	{
		auto s = iso_util::read_system_cnf( iso_path );
		( void )s;
	}
	catch ( const std::exception& )
	{
		threw = true;
	}

	assert( threw && "Expected read_system_cnf to throw when SYSTEM.CNF not present" );
	filesystem::remove( iso_path );
	cout << "missing SYSTEM.CNF test passed\n";
}

void test_truncated_dir()
{
	const filesystem::path iso_path = "test_iso_truncated.iso";
	const uint32_t root_lba = 20;
	{
		ofstream f(iso_path, ios::binary | ios::trunc);
		assert(f);
		f.seekp((root_lba + 2) * sector_size - 1, ios::beg);
		char z = 0; f.write(&z,1);
		vector<char> pvd(sector_size,0);
		pvd[0]=1; memcpy(pvd.data()+1, "CD001",5); pvd[6]=1;
		const size_t RDR_OFF = 0x9c;
		vector<uint8_t> rdr(34,0);
		rdr[2] = static_cast<uint8_t>(root_lba & 0xFF);
		rdr[3] = static_cast<uint8_t>((root_lba >> 8) & 0xFF);
		rdr[4] = static_cast<uint8_t>((root_lba >> 16) & 0xFF);
		rdr[5] = static_cast<uint8_t>((root_lba >> 24) & 0xFF);
		memcpy(pvd.data()+RDR_OFF, rdr.data(), rdr.size());
		write_sector(f,16,pvd);

		// Put a very small record (len = 5) in root dir
		vector<char> dir(sector_size,0);
		dir[0] = 5; // too small, should be skipped
		write_sector(f, root_lba, dir);
		f.flush(); f.close();
	}

	bool threw = false;
	try { auto s = iso_util::read_system_cnf(iso_path); (void)s; }
	catch (const std::exception&) { threw = true; }
	assert(threw);
	filesystem::remove(iso_path);
	cout << "truncated dir record test passed\n";
}

void test_large_idlen()
{
	const filesystem::path iso_path = "test_iso_largeid.iso";
	const uint32_t root_lba = 20;
	const uint32_t file_lba = 30;
	const string syscnf = "BOOT = cdrom0:\\BIGID;1\n";
	vector<char> sysbuf(syscnf.begin(), syscnf.end());
	{
		ofstream f(iso_path, ios::binary | ios::trunc);
		assert(f);
		f.seekp((file_lba + 2) * sector_size - 1, ios::beg);
		char z = 0; f.write(&z,1);
		vector<char> pvd(sector_size,0);
		pvd[0]=1; memcpy(pvd.data()+1, "CD001",5); pvd[6]=1;
		const size_t RDR_OFF = 0x9c;
		vector<uint8_t> rdr(34,0);
		rdr[2] = static_cast<uint8_t>(root_lba & 0xFF);
		rdr[3] = static_cast<uint8_t>((root_lba >> 8) & 0xFF);
		rdr[4] = static_cast<uint8_t>((root_lba >> 16) & 0xFF);
		rdr[5] = static_cast<uint8_t>((root_lba >> 24) & 0xFF);
		memcpy(pvd.data()+RDR_OFF, rdr.data(), rdr.size());
		write_sector(f,16,pvd);

		vector<char> dir(sector_size,0);
		// set record length large, and set id_len bigger than remaining record size
		dir[0] = 200; // artificially large
		dir[2] = static_cast<char>(file_lba & 0xFF);
		dir[3] = static_cast<char>((file_lba >> 8) & 0xFF);
		dir[4] = static_cast<char>((file_lba >> 16) & 0xFF);
		dir[5] = static_cast<char>((file_lba >> 24) & 0xFF);
		// put id_len very large so our code must cap it
		dir[32] = 180;
		// put a short ident actually in available space
		string ident = "SYSTEM.CNF;1";
		memcpy(dir.data()+33, ident.data(), ident.size());
		write_sector(f, root_lba, dir);

		// write system.cnf data at file_lba
		vector<char> filedata = sysbuf;
		filedata.resize(((filedata.size() + sector_size -1)/sector_size)*sector_size,0);
		write_sector(f, file_lba, filedata);

		f.flush(); f.close();
	}

	string got = iso_util::read_system_cnf(iso_path);
	assert(got.find("BIGID") != string::npos);
	filesystem::remove(iso_path);
	cout << "large id_len test passed\n";
}

void test_joliet_filename( )
{
	const filesystem::path iso_path = "test_iso_joliet.iso";
	const uint32_t root_lba = 20;
	const uint32_t root_size = 1 * sector_size;
	const uint32_t file_lba = 30;

	// Build an ISO where root contains a Joliet (UCS-2 BE) filename for SYSTEM.CNF
	const string syscnf = "BOOT = cdrom0:\\JOLIET_ID;1\n";
	vector<char> sysbuf( syscnf.begin( ), syscnf.end( ) );

	{
		ofstream f( iso_path, ios::binary | ios::trunc );
		assert( f );
		uint64_t last_sector = file_lba + 4;
		f.seekp( ( last_sector + 1 ) * sector_size - 1, ios::beg );
		char z = 0; f.write( &z, 1 );

		vector<char> pvd( sector_size, 0 );
		pvd[ 0 ] = 1; memcpy( pvd.data( ) + 1, "CD001", 5 ); pvd[ 6 ] = 1;
		const size_t RDR_OFF = 0x9c;
		vector<uint8_t> rdr( 34, 0 );
		rdr[ 2 ] = static_cast< uint8_t >( root_lba & 0xFF );
		rdr[ 3 ] = static_cast< uint8_t >( ( root_lba >> 8 ) & 0xFF );
		rdr[ 4 ] = static_cast< uint8_t >( ( root_lba >> 16 ) & 0xFF );
		rdr[ 5 ] = static_cast< uint8_t >( ( root_lba >> 24 ) & 0xFF );
		rdr[ 10 ] = static_cast< uint8_t >( root_size & 0xFF );
		rdr[ 11 ] = static_cast< uint8_t >( ( root_size >> 8 ) & 0xFF );
		rdr[ 12 ] = static_cast< uint8_t >( ( root_size >> 16 ) & 0xFF );
		rdr[ 13 ] = static_cast< uint8_t >( ( root_size >> 24 ) & 0xFF );
		memcpy( pvd.data( ) + RDR_OFF, rdr.data( ), rdr.size( ) );
		write_sector( f, 16, pvd );

		// Create a dir entry where filename is UCS-2 BE: 0x00 'S', 0x00 'Y', ... etc
		vector<char> dir( sector_size, 0 );
		string ident = "SYSTEM.CNF;1";
		// Build UCS-2 BE representation
		vector<char> ucs2;
		for ( char c : ident )
		{
			ucs2.push_back( 0 ); ucs2.push_back( c );
		}
		uint8_t id_len = static_cast< uint8_t >( ucs2.size( ) );
		uint8_t rec_len = static_cast< uint8_t >( 33 + id_len );
		dir[ 0 ] = rec_len;
		uint32_t file_lba_le = file_lba;
		dir[ 2 ] = static_cast< char >( file_lba_le & 0xFF );
		dir[ 3 ] = static_cast< char >( ( file_lba_le >> 8 ) & 0xFF );
		dir[ 4 ] = static_cast< char >( ( file_lba_le >> 16 ) & 0xFF );
		dir[ 5 ] = static_cast< char >( ( file_lba_le >> 24 ) & 0xFF );
		uint32_t file_size = static_cast< uint32_t >( sysbuf.size( ) );
		dir[ 10 ] = static_cast< char >( file_size & 0xFF );
		dir[ 11 ] = static_cast< char >( ( file_size >> 8 ) & 0xFF );
		dir[ 12 ] = static_cast< char >( ( file_size >> 16 ) & 0xFF );
		dir[ 13 ] = static_cast< char >( ( file_size >> 24 ) & 0xFF );
		dir[ 32 ] = id_len;
		memcpy( dir.data( ) + 33, ucs2.data( ), ucs2.size( ) );
		write_sector( f, root_lba, dir );

		vector<char> filedata = sysbuf;
		filedata.resize( ( ( filedata.size( ) + sector_size - 1 ) / sector_size ) * sector_size, 0 );
		write_sector( f, file_lba, filedata );

		f.flush( ); f.close( );
	}

	string got = iso_util::read_system_cnf( iso_path );
	assert( got.find( "JOLIET_ID" ) != string::npos );
	filesystem::remove( iso_path );
	cout << "joliet filename test passed\n";
}

int main( )
{
	try
	{
		test_extract_boot_id( );
		test_read_system_cnf_minimal( );
		test_missing_system_cnf( );
		test_truncated_dir();
		test_large_idlen();
		test_joliet_filename( );
	}
	catch ( const std::exception& e )
	{
		cerr << "Test failed: " << e.what( ) << '\n';
		return 2;
	}
	cout << "All tests passed\n";
	return 0;
}
