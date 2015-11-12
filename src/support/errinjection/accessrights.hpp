

class AccessRights
{
	private:
		bool read :1;
		bool write :1;
		bool execution :1;

	public:
		constexpr
		AccessRights() :
				read(false),
				write(false),
				execution(false)
		{}

		constexpr
		AccessRights( unsigned short access_mask ) :
				read( access_mask & 1<<2 ),
				write( access_mask & 1<<1 ),
				execution( access_mask & 1<<0 )
		{
			static_assert( access_bits < 7, "Illegal access rights mask." );
		}

		constexpr
		AccessRights( bool readable, bool writable, bool executable ) :
				read( readable ),
				write( writable ),
				execution( executable )
		{}

		constexpr bool isReadable() const { return read; }
		constexpr bool isWritable() const { return write; }
		constexpr bool isExecutable() const { return execution; }

		void setReadable( bool flag ) { read = flag; }
		void setWritable( bool flag ) { write = flag; }
		void setExecutable( bool flag ) { execute = flag; }
};

std::ostream& operator<<(std::ostream& out, AccessRights const &access)
{
	std::string protection( "---" );

	if( access.isReadable() )
		protection[0] = 'r';

	if( access.isWritable() )
		protection[1] = 'w';

	if( access.isExecutable() )
		protection[2] = 'x';

	return out << protection;
}

std::istream& operator>>(std::istream& input, AcessRights &access)
{
	access.setReadable( input.get() == 'r' );
	access.setWritable( input.get() == 'w' );
	access.setExecutable( input.get() == 'x' );
}

