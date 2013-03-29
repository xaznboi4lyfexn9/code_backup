/**
 *test_gif.c
 * Decodes gif images.
 *
 *
 * Taken from iiview project, which is also
 * located on sourceforge. Please look there for
 * more details about this file.
 * ffffffffffffffffffffffffffffffff
 * ####E4K3lb3I7H7coBZAfFU0wCjw####
 * ####U9CUniesYbVX0v9BUQSJutbQ####
 * ####XkNEVg==                ####
 * ffffffffffffffffffffffffffffffff
 **/
#if 0
#include "AEE.h"
#include "AEEStdLib.h"     // StdLib functions
#include "AEEFile.h"
#include "gif.h"
#include "portcommon.h"

#define M_TRAILER     0x3B
#define M_IMAGE       0x2C
#define M_EXTENSION   0x21

/* prototypes */
void readColormap(GIF_PTR* gif_ptr,int32);



int32 int_gif_readchar(GIF_PTR* gif_ptr) 
{
	int32 buffer=0;

	if(gif_ptr->gif_input.flag==1)
	{
              if(gif_ptr->gif_input.bufpos>=gif_ptr->gif_input.buflen)
			return -1;
			  
		buffer |= gif_ptr->gif_input.buf[gif_ptr->gif_input.bufpos];
		gif_ptr->gif_input.bufpos++;
	}
	return buffer;
}



static int32 gif_readword(GIF_PTR* gif_ptr)
{
  	return ((int_gif_readchar(gif_ptr) & 0xff)|((int_gif_readchar(gif_ptr)&0xff)<<8))&0xFFFF;
}



void readColormap(GIF_PTR* gif_ptr,int32 colors) 
{
	int32 i;
	
	if(colors>256)
		colors=256;
	
	for (i=0;i<colors;i++)
	gif_ptr->gif_pal[i]=gif_readword(gif_ptr)|(int_gif_readchar(gif_ptr)<<16);
}



static int32 readCode(GIF_PTR* gif_ptr)
{
	int32 size;
	int32 i;
	int32 bits;
	bits=size=0;

	while (size<gif_ptr->codesize) 
	{
	   if (gif_ptr->bitpos>=gif_ptr->maxbits) 
	   {
	          if ((gif_ptr->maxbits=int_gif_readchar(gif_ptr))==-1) 
	  	     return -1;  
		gif_ptr->maxbits<<=3;
	  		for (i=0;i<gif_ptr->maxbits>>3;i++) 
	  		{
			gif_ptr->bitbuf[i]=int_gif_readchar(gif_ptr);
	 	 	}
	  		gif_ptr->bitpos=0;
	   }
	   
	   bits=bits|(((gif_ptr->bitbuf[gif_ptr->bitpos>>3]>>(gif_ptr->bitpos&7))&1)<<size);
	   gif_ptr->bitpos++;size++;
	   
	}

	return bits & gif_ptr->codemask;
}



static void saveColor(GIF_PTR* gif_ptr,int32 nr)
{
	if (gif_ptr->data-gif_ptr->gif_data>=gif_ptr->gif_width*gif_ptr->gif_height) 
	return;

	*(gif_ptr->data)++=nr;

	if (gif_ptr->gif_interlaced) 
	{
	   if ((++gif_ptr->gif_ipixel)>=gif_ptr->gif_width) 
	{
	  		gif_ptr->gif_ipixel=0;
	  		gif_ptr->gif_iindex+=gif_ptr->gif_ipassinc;
	  		if (gif_ptr->gif_iindex>=gif_ptr->gif_height) 
		{
			switch (++gif_ptr->gif_ipassnr) 
			{
				case 1:
				  gif_ptr->gif_ipassinc=8;
				  gif_ptr->gif_iindex=4;
				  break;
				case 2:
				  gif_ptr->gif_ipassinc=4;
				  gif_ptr->gif_iindex=2;
				  break;
				case 3:
				  gif_ptr->gif_ipassinc=2;
				  gif_ptr->gif_iindex=1;
				  break;
			}
	  		}
	  		gif_ptr->data=gif_ptr->gif_data+gif_ptr->gif_iindex*gif_ptr->gif_width;
		}
	}
}



static int32 readImage(GIF_PTR* gif_ptr)
{
	int32 prefix[4096];
	unsigned char suffix[4096],output[4096];
	int32 leftoffs,topoffs,info,CC,EOFC;
	int32 code,lastcode=0,lastchar=0,curcode,savecode;
	int32 freecode,initfreecode,outcodenr;


	leftoffs=gif_readword(gif_ptr);
	topoffs=gif_readword(gif_ptr);
	gif_ptr->gif_width=gif_readword(gif_ptr);
	gif_ptr->gif_height=gif_readword(gif_ptr);

	MaolInfo("gif_width:%d gif_height:%d==========",gif_ptr->gif_width,gif_ptr->gif_height);

	gif_ptr->gif_data=(unsigned char *)MaolMalloc(gif_ptr->gif_width*(gif_ptr->gif_height+1));

	if (!gif_ptr->gif_data) 
	{
	  MaolInfo("readImage:Out of memory bufsize=%d",gif_ptr->gif_width*(gif_ptr->gif_height+1));
	  return 0;
	}
	MEMSET(gif_ptr->gif_data,0,gif_ptr->gif_width*(gif_ptr->gif_height+1));
	info=int_gif_readchar(gif_ptr);
	gif_ptr->gif_interlaced=info&0x40;

	if (info&0x80) 
		readColormap(gif_ptr,(int32)1 <<((info&7)+1));

	gif_ptr->data=gif_ptr->gif_data;
	gif_ptr->initcodesize=int_gif_readchar(gif_ptr);
	gif_ptr->bitpos=gif_ptr->maxbits=0;

	CC=1<<gif_ptr->initcodesize;
	EOFC=CC+1;
	freecode=initfreecode=CC+2;

	gif_ptr->codesize=++gif_ptr->initcodesize;
	gif_ptr->codemask=(1<<gif_ptr->codesize)-1;
	gif_ptr->gif_iindex=gif_ptr->gif_ipassnr=0;
	gif_ptr->gif_ipassinc=8;
	gif_ptr->gif_ipixel=0;

	if ((code=readCode(gif_ptr))>=0) 
	while (code!=EOFC) 
	{
		if (code==CC) 
		{
		      gif_ptr->codesize=gif_ptr->initcodesize;
		      gif_ptr->codemask=(1<<gif_ptr->codesize)-1;
		      freecode=initfreecode;
		      lastcode=lastchar=code=readCode(gif_ptr);
		      if (code<0) break;
		      saveColor(gif_ptr,code);
		} 
	else 
	{
	   	outcodenr=0;savecode=code;
	    	if (code>=freecode) 
		{
			if (code>freecode) break;
			output[(outcodenr++) & 0xFFF]=lastchar;
			code=lastcode;
	      	}
	      	curcode=code;

	       while (curcode>gif_ptr->gif_palmask) 
		{
			if (outcodenr>4095) break;
			output[(outcodenr++)&0xFFF]=suffix[curcode];
			curcode=prefix[curcode];
	      	}
	  
	      	output[(outcodenr++)&0xFFF]=lastchar=curcode;

	      	while (outcodenr>0)
			saveColor(gif_ptr,output[--outcodenr]);

	      prefix[freecode]=lastcode;
	      suffix[freecode++]=lastchar;
	      lastcode=savecode;

	      	if (freecode>gif_ptr->codemask) 
		{
			if (gif_ptr->codesize<12) 
			{
		  		gif_ptr->codesize++;
		  		gif_ptr->codemask=(gif_ptr->codemask<<1) |1;
			}
	      	}
	}

	if ((code=readCode(gif_ptr))<0) 
	break;

	}

	return 1;

}



/* output size will be ignored */
int32 GIF_OpenStream(GIF_PTR* gif_ptr)
{
	int32 i,marker,gifctlex;
	const char gifmarker[]={'G','I','F'};
	int32 gif_gctdef=0;
	int32 gif_resolution=0;
	int32 gif_colors=0;
	int32 gif_background=0;
	int32 gif_aspectratio=0;

	gif_ptr->gif_data=NULL;
	gif_ptr->gif_pal=NULL;
	gif_ptr->gif_width=gif_ptr->gif_height=0;

	for (i=0;i<3;i++)
	if (int_gif_readchar(gif_ptr)!=gifmarker[i]) 
	{
	    MaolInfo("int_gif_readchar failed!i=%d",i);
	    return 0;
	}

	// skip version info for now
	for (i=0;i<3;i++)
	int_gif_readchar(gif_ptr);

	gif_ptr->gif_width=gif_readword(gif_ptr);
	gif_ptr->gif_height=gif_readword(gif_ptr);

	i=int_gif_readchar(gif_ptr);
	gif_gctdef=i&0x80;
	gif_resolution=((i>>4)&7)+1;
	gif_colors=(int32)1 << ((i&7)+1);
	gif_ptr->gif_palmask=gif_colors-1;

	gif_background=int_gif_readchar(gif_ptr);
	gif_aspectratio=int_gif_readchar(gif_ptr);

	if (gif_aspectratio) 
	{
	MaolInfo("Hmm: ASPECT RATIO given ! What to do ?");
		return 0;
	}

	gif_ptr->gif_pal=(int32*)MaolMalloc(256*sizeof(int32));
	if (gif_gctdef) readColormap(gif_ptr,gif_colors);

	do 
	{
	if ((marker=int_gif_readchar(gif_ptr))==-1) break;

	switch (marker) 
	{
		case M_IMAGE: 
	      if(readImage(gif_ptr)==0)
	      {
	        	MaolInfo("readImage:failed");
			return 0;	
	      }
	      int_gif_readchar(gif_ptr);
	         break;
			  
	case M_EXTENSION:
	      gifctlex = int_gif_readchar(gif_ptr);
	      i=int_gif_readchar(gif_ptr);
	      while (i) 
	      {	
			while (i--) 
			{	
				if(i==4)
				{
					if((int_gif_readchar(gif_ptr)&0x01)!=0)
						gif_ptr->transenable = 1;
					else
						gif_ptr->transenable = 0;
					continue;
				}
				if(i==1)
				{
					gif_ptr->transcoloridex = int_gif_readchar(gif_ptr);
					continue;
				}
				int_gif_readchar(gif_ptr);
			}
			i=int_gif_readchar(gif_ptr);
		 }
	  		 break;
			
	case M_TRAILER:
	  break;
	  
	default:
	i=int_gif_readchar(gif_ptr);
	while (i) 
	{
		while (i--) 
			int_gif_readchar(gif_ptr);
		
		i=int_gif_readchar(gif_ptr);
	}
	break;

	}

	} while (marker!=M_TRAILER);

	return 1;
}



void GIF_CloseStream(GIF_PTR* gif_ptr)
{
	if (gif_ptr->gif_pal!=NULL) 
	{
  		MaolFree(gif_ptr->gif_pal);
  		gif_ptr->gif_pal = NULL;
	}

	if (gif_ptr->gif_data!=NULL) 
	{
  		MaolFree(gif_ptr->gif_data);
  		gif_ptr->gif_data = NULL;
	}

	if(gif_ptr->gif_input.fd>0) 
	{
              ioCloseRaw(gif_ptr->gif_input.fd);
  		gif_ptr->gif_input.fd = -1;
	}
	if(gif_ptr->gif_input.buf!=NULL)
	{
	    	MaolFree(gif_ptr->gif_input.buf);
	    	gif_ptr->gif_input.buf = NULL;  
	}

	//MaolFree(gif_ptr);
}



int32 GIF_GetPixel(GIF_PTR* gif_ptr,int32 x,int32 y,int32 col)
{
	int32 color;

	if ((x>=gif_ptr->gif_width)||(y>=gif_ptr->gif_height))
	return 0x000000;

	color=gif_ptr->gif_pal[gif_ptr->gif_data[y*gif_ptr->gif_width+x]];

	return col ? (color&0xFF00)|((color>>16)&0xFF)|((color&0xFF)<<16) : 
	(((color&0xFF)+((color>>8)&0xFF)+((color>>16)&0xFF))/3)*0x010101;
}



int32 GIF_GetWidth(GIF_PTR* gif_ptr)
{
  	return gif_ptr->gif_width;
}



int32 GIF_GetHeight(GIF_PTR* gif_ptr)
{
  	return gif_ptr->gif_height;
}



int32 GIF_Decode(GIF_PTR* gif_ptr,void* buf,int32 buflen,int32 isfile)
{
	int32 fd=NULL;
	
	MaolInfo("filename:%s",buf);

	if(gif_ptr==NULL)
		return 0;

	if(isfile)
	{
        	int32 size;

		fd = ioOpenRaw(buf,MC_OF_READ);
		if(fd==NULL)
		{
                     MaolInfo("GIF_Decode:ioOpenRaw faild!");
			return 0;
		}
		
              size = ioSizeRaw(fd);
              gif_ptr->gif_input.buf = (unsigned char*)MaolMalloc(size+4);
			  
		if(gif_ptr->gif_input.buf==NULL)
			return 0;
		
              ioReadRaw(fd, gif_ptr->gif_input.buf, size);
			  
              gif_ptr->gif_input.fd=-1;
              gif_ptr->gif_input.flag=1;
              gif_ptr->gif_input.bufpos = 0;
		gif_ptr->gif_input.buflen = size;
              ioCloseRaw(fd);
	}
	else
	{
		gif_ptr->gif_input.buf = (unsigned char*)MaolMalloc(buflen+4);              
		MEMCPY(gif_ptr->gif_input.buf,buf,buflen);
		gif_ptr->gif_input.flag = 1;
		gif_ptr->gif_input.bufpos = 0;
		gif_ptr->gif_input.buflen = buflen;
	}

	if(GIF_OpenStream(gif_ptr)==1)
	{
        	MaolInfo("GIF_OpenStream:success++++++++++");
        	return 1;
       }
	else
	{
        	MaolInfo("GIF_OpenStream:failed");
		return 0;
	}
}



int32 GIF_GetTransColor(GIF_PTR* gif_ptr,int32* transcolor)
{
	if(!gif_ptr->transenable)
		return 0;
	
	*transcolor =gif_ptr->gif_pal[gif_ptr->transcoloridex];
	
	return 1;
}
#endif
