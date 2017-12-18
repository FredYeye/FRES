#include <array>
#include <vector>


const std::array<uint32_t, 5> SHA1(std::vector<uint8_t> message)
{
	uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;

	const uint64_t ml = message.size() * 8;
	message.push_back(0x80);
	while((message.size() + 8) & 63)
	{
		message.push_back(0);
	}
	for(int x = 7; x > -1; --x)
	{
		message.push_back(uint8_t(ml >> x * 8));
	}

	for(int x = 0; x < message.size() >> 6; ++x)
	{
		std::array<uint32_t, 80> w;
		for(int y = 0; y < 16; ++y)
		{
			w[y]  = message[x * 64 + y * 4 + 3];
			w[y] |= message[x * 64 + y * 4 + 2] << 8;
			w[y] |= message[x * 64 + y * 4 + 1] << 16;
			w[y] |= message[x * 64 + y * 4    ] << 24;
		}

		for(int i = 16; i < 80; ++i)
		{
			w[i] = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
			w[i] = (w[i] << 1) | (w[i] >> 31);
		}

		uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

		for(int y = 0; y < 80; ++y)
		{
			uint32_t f, k;
			if(y < 20)
			{
				f = (b & c) | ((~b) & d);
				k = 0x5A827999;
			}
			else if(y < 40)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}
			else if(y < 60)
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}
			else
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			uint32_t temp = (a << 5) | (a >> 27);
			temp += f + e + k + w[y];

			e = d;
			d = c;
			c = (b << 30) | (b >> 2);
			b = a;
			a = temp;
		}
		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
	}

	return {h0, h1, h2, h3, h4};
}
