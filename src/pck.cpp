/*
This file is part of "UFO 2000" aka "X-COM: Gladiators"
                    http://ufo2000.sourceforge.net/
Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002       ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"

#include "global.h"
#include "video.h"
#include "pck.h"

#undef map

//! We cache loaded pck files here
static std::map<std::string, PCK *> g_pck_cache;

/**
 * Load image from pck file
 */
BITMAP *pck_image(const char *filename, int index)
{
	std::string fullname = F(filename);
	if (g_pck_cache.find(fullname) == g_pck_cache.end())
		g_pck_cache[fullname] = new PCK(fullname.c_str());
	return g_pck_cache[fullname]->get_image(index);
}

/**
 * Load image from pck file
 */
BITMAP *pck_image_ex(bool tftd_flag, int width, int height, const char *filename, int index)
{
	std::string fullname = F(filename);
	if (g_pck_cache.find(fullname) == g_pck_cache.end())
		g_pck_cache[fullname] = new PCK(fullname.c_str(), tftd_flag, width, height);
	PCK *pck = g_pck_cache[fullname];
	ASSERT(pck->m_tftd_flag == tftd_flag && width == pck->m_width && height == pck->m_height);
	return pck->get_image(index);
}

/**
 * Free all loaded pck files (used at exit)
 */
void free_pck_cache()
{
	std::map<std::string, PCK *>::iterator it = g_pck_cache.begin();
	while (it != g_pck_cache.end()) {
		delete it->second;
		it++;
	}
}

PCK::PCK(const char *pckfname, int tftd_flag, int width, int height)
{
	m_tftd_flag = tftd_flag;
	m_imgnum = 0;
	m_width = width;
	m_height = height;
	loadpck(pckfname, width, height);
    if (FLAGS & F_CONVERT_XCOM_DATA) {
		std::string filename = get_filename(pckfname);
		std::string dir = F("$(home)/converted_xcom_data");
#ifdef LINUX
        mkdir(dir.c_str(), 0755);
#else
        mkdir(dir.c_str());
#endif
        std::string bmp_name = dir + "/" + filename + ".bmp";
        save_as_bmp(bmp_name.c_str());
    }
}

PCK::~PCK()
{
	for (unsigned int i = 0; i < m_bmp.size(); i++) {
		ASSERT(m_bmp[i] != NULL);
		if (m_bmp[i] != NULL)
			destroy_bitmap(m_bmp[i]);
	}
}

/**
 * Decode single frame from PCK file and store it in bitmap
 *
 * @param data pointer to data buffer for a frame from PCK file
 * @param size size of frame data
 * @param tftd_flag flag specifying whether we need to use tftd palette
 * @return     bitmap with a frame image
 */
BITMAP *PCK::pckdat2bmp(const unsigned char *data, int size, int width, int height, int tftd_flag)
{   
	BITMAP *bmp = create_bitmap(width, height);
	clear_to_color(bmp, xcom1_color(0));

	long ofs = ((int)*data * width);

	if (data[--size] != 0xFF) ASSERT(false);

	for (int j = 1; j < size; j++) {
		switch (data[j]) {
			case 0xFE: 
				ofs += data[++j]; break;
			case 0x00: 
				ofs++; break;
			default:
				ASSERT(ofs / width < height);
				putpixel(bmp, ofs % width, ofs / width, 
					tftd_flag ? tftd_color(data[j]) : xcom1_color(data[j]));
				ofs++;
				break;
		}
	}

	return bmp;
}

/**
 * Load all frames from .pck file and append them to already loaded set
 *
 * @param pckfname name of PCK file to be loaded
 * @return         number of frames loaded
 */
int PCK::loadpck(const char *pckfname, int width, int height)
{
	int i;                   

	int fh = open(F(pckfname), O_RDONLY | O_BINARY);
	ASSERT(fh != -1);
	long pcksize = filelength(fh);
	unsigned char *pck = new unsigned char[pcksize];
	read(fh, pck, pcksize);
	close(fh);

	strcpy(m_fname, pckfname);
	strcpy(strrchr(m_fname, '.') + 1, "tab");
	fh = open(F(m_fname), O_RDONLY | O_BINARY);

	if (fh == -1) {
    //	Just a single frame from .pck file
		m_imgnum = 1;
		m_bmp.resize(m_imgnum);
		m_bmp[0] = pckdat2bmp(pck, pcksize, width, height, m_tftd_flag);
		delete [] pck;
		return 1;
	} 

	long tabsize = filelength(fh);
	unsigned char *tabdata = new unsigned char[tabsize + 4];
	read(fh, tabdata, tabsize);
	close(fh);

	if (*(uint32 *)tabdata == 0x00000000) {
    //	32-bit records in .tab file (UFO2)
		uint32 *tab = (uint32 *)tabdata;
		m_imgnum = tabsize / 4;
		for (i = 0; i < m_imgnum; i++)
			tab[i] = intel_uint32(tab[i]);
		tab[m_imgnum] = pcksize;
		m_bmp.resize(m_imgnum);
		for (i = 0; i < m_imgnum; i++)
			m_bmp[i] = pckdat2bmp(&pck[tab[i]], tab[i + 1] - tab[i], width, height, m_tftd_flag);
	} else {
    //	16-bit records in .tab file (UFO1)
		uint16 *tab = (uint16 *)tabdata;
		m_imgnum = tabsize / 2;
		for (i = 0; i < m_imgnum; i++)
			tab[i] = intel_uint16(tab[i]);
		tab[m_imgnum] = pcksize;
		m_bmp.resize(m_imgnum);
		for (i = 0; i < m_imgnum; i++)
			m_bmp[i] = pckdat2bmp(&pck[tab[i]], tab[i + 1] - tab[i], width, height, m_tftd_flag);
	}

	delete [] pck;
	delete [] tabdata;

	return m_imgnum;
}

void PCK::showpck(int num, int xx, int yy)
{
	if (num >= m_imgnum) {
		return;
	}
	draw_sprite(screen2, m_bmp[num], xx, yy - 6);
}

void PCK::showpck(BITMAP *img, int xx, int yy)
{
	draw_sprite(screen2, img, xx, yy - 6);
}

void PCK::showpck(BITMAP *dest, BITMAP *img, int xx, int yy)
{
	draw_sprite(dest, img, xx, yy - 6);
}

void PCK::drawpck(int num, BITMAP *dest, int y)
{
	ASSERT(num < m_imgnum);
	draw_sprite(dest, m_bmp[num], 0, y - 6);
}

/* Parameters for saving data in BMP format */

#define SIZE 8

/**
 * Function for debugging and dumping .pck files only
 *
 * @param fname name of the file to which this PCK file is saved in BMP format
 */
void PCK::save_as_bmp(const char *fname)
{
	int rows = ((m_imgnum + SIZE - 1) / SIZE);
	BITMAP *bmp = create_bitmap(m_width * SIZE + SIZE + 1, m_height * rows + rows + 1);
	clear_to_color(bmp, xcom_color(0));
    for (int x = 0; x < bmp->w; x += m_width + 1) vline(bmp, x, 0, bmp->h - 1, makecol(0, 0, 0));
    for (int y = 0; y < bmp->h; y += m_height + 1) hline(bmp, 0, y, bmp->w - 1, makecol(0, 0, 0));

	for (int i = 0; i < m_imgnum; i++)
		draw_sprite(bmp, m_bmp[i], (i % SIZE) * (m_width + 1) + 1, (i / SIZE) * (m_height + 1) + 1);

	save_bitmap(fname, bmp, (RGB *)datafile[DAT_GAMEPAL_BMP].dat);
	destroy_bitmap(bmp);
}
