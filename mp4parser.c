#include "stdafx.h"
#include "mp4.h"
#include "malloc.h"
#include "string.h"


int main(int argc, char* argv[])
{
   int sampleCount;
   mp4_box_t *root = NULL, *stsz = NULL, *stco = NULL, *stsc = NULL;
   stream_t* s = NULL;

   //////////////ÐÞ¸Äºó
   unsigned long filesize = 0;
   BUFFER_t *buffer = NULL;
   FILE *file = fopen("test.mp4","rb");
   fseek(file,0L,SEEK_END);
   filesize = ftell(file);
   fseek(file,0L,SEEK_SET); 
   buffer = (BUFFER_t *)malloc(sizeof(BUFFER_t));
   buffer->begin_addr = (unsigned char *)malloc(filesize);
   buffer->buf = (unsigned char *)malloc(filesize);
   fread(buffer->begin_addr,filesize,1,file);
   memcpy(buffer->buf,buffer->begin_addr,filesize);
   (*buffer).offset = 0;
   (*buffer).filesize = filesize;
   s = create_buffer_stream();
   if (buffer_open(s, buffer) == 0)
      return -1;
  
   root = MP4_BoxGetRootFromBuffer(s,filesize);
   stsz = MP4_BoxSearchBox(root,ATOM_stsz);
   printf("box is %c%c%c%c  type %x  sample_count %d\n"
		,stsz->i_type&0x000000ff
		,(stsz->i_type&0x0000ff00)>>8
		,(stsz->i_type&0x00ff0000)>>16
		,(stsz->i_type&0xff000000)>>24
		,stsz->i_type
		,stsz->data.p_stsz->sample_count);
		
   int stsz_sample_count = stsz->data.p_stsz->sample_count;
   int stsz_smaple_size[stsz_sample_count];
   for(sampleCount = 0; sampleCount < stsz_sample_count; sampleCount++)
   {
	   stsz_smaple_size[sampleCount] = stsz->data.p_stsz->entry_size[sampleCount];
	   //printf("No %d smaple size %d\n",sampleCount+1,stsz_smaple_size[sampleCount]);
   }
   stco = MP4_BoxSearchBox(root,ATOM_stco);
   printf("box is %c%c%c%c  type %x  sample_count %d\n"
		,stco->i_type&0x000000ff
		,(stco->i_type&0x0000ff00)>>8
		,(stco->i_type&0x00ff0000)>>16
		,(stco->i_type&0xff000000)>>24
		,stco->i_type
		,stco->data.p_stco->sample_size);
		
   int stco_entry_count = stco->data.p_stco->sample_size;
   int stco_chunk_offset[stco_entry_count];
   for(sampleCount = 0; sampleCount < stco_entry_count; sampleCount++) 
   {
	   stco_chunk_offset[sampleCount] = stco->data.p_stco->entry_size[sampleCount*2];
  	   //printf("No %d Chunk offset %d\n",sampleCount+1,stco_chunk_offset[sampleCount]);
   }
   	
   stsc = MP4_BoxSearchBox(root,ATOM_stsc);
   printf("box is %c%c%c%c  type %x  entry_count %d\n"
		,stsc->i_type&0x000000ff
		,(stsc->i_type&0x0000ff00)>>8
		,(stsc->i_type&0x00ff0000)>>16
		,(stsc->i_type&0xff000000)>>24
		,stsc->i_type
		,stsc->data.p_stsc->entry_count);
	
	int stsc_entry_count = stsc->data.p_stsc->entry_count;
	int stsc_first_chunk[stsc_entry_count];
	int stsc_samples_per_chunk[stsc_entry_count];
	for(sampleCount = 0; sampleCount < stsc_entry_count; sampleCount++) 
    {
	   stsc_first_chunk[sampleCount] = stsc->data.p_stsc->first_chunk[sampleCount];
	   stsc_samples_per_chunk[sampleCount] = stsc->data.p_stsc->samples_per_chunk[sampleCount];
  	   //printf("No %d Chunk offset %d\n",sampleCount+1,stco_chunk_offset[sampleCount]);
    }
   	
   MP4_BoxFreeFromBuffer( root );
   buffer_close(s);
   destory_buffer_stream(s);
      
   ////////////////
   
   s = create_file_stream();
   if (stream_open(s, "test.mp4", MODE_READ) == 0)
      return -1;
  
   int len;
   int buff[1] = {0};
   int buff_tmp[4] = {0, 0, 0, 0};
   
   int avcC_type[4] = {0x43, 0x63, 0x76, 0x61};
   int avcC_start_offset = 0;
   
   int sps_len_offset[2] = {0, 0};
   int sps_len = 0;
   int sps_start_offset = 0;
   int sps_len_start = 0;
   int sps[1024] = {0,0,0,1}; // h264 start code 0001
    
   int pps_len_offset[2] = {0, 0};
   int pps_len = 0;
   int pps_start_offset = 0;
   int pps_len_start = 0;
   int pps[1024] = {0,0,0,1}; // h264 start code 0001

   int offset = 0;
   int startCodeNum = 4;

   while (1)
   {
      len = stream_read(s, buff, 1);
	  buff_tmp[3] = buff_tmp[2];
	  buff_tmp[2] = buff_tmp[1];
	  buff_tmp[1] = buff_tmp[0];
	  buff_tmp[0] = buff[0];
	  //printf("%x ",buff[0]);
	  if(buff_tmp[0] == avcC_type[0] && buff_tmp[1] == avcC_type[1] && buff_tmp[2] == avcC_type[2] && buff_tmp[3] == avcC_type[3])
	  {
		  avcC_start_offset = offset;
		  sps_len_offset[0] = avcC_start_offset + 7;
		  sps_len_offset[1] = sps_len_offset[0] + 1;
		  sps_start_offset = sps_len_offset[1] + 1;
		  //printf("offset %d\n",offset);
	  }
	  // get sps
	  if(offset == sps_len_offset[0] || offset == sps_len_offset[1])
	  {
		  if(offset == sps_len_offset[0])
		  {
			  sps_len = buff[0] << 8;
		  } 
		  else 
		  {
			  sps_len += buff[0];
			  pps_len_offset[0] = sps_start_offset + sps_len + 1;
			  pps_len_offset[1] = pps_len_offset[0] + 1;
			  pps_start_offset = pps_len_offset[1] + 1;
		  }
	  }
	  if(offset >= sps_start_offset && offset <= sps_start_offset + sps_len - 1)
	  {
		  sps[sps_len_start+startCodeNum] = buff[0];
		  sps_len_start++;
		  //printf("%x ",buff[0]);
	  }
	  // get pps
	  if(offset == pps_len_offset[0] || offset == pps_len_offset[1]) 
	  {
		  if(offset == pps_len_offset[0])
		  {
			  pps_len = buff[0] << 8;
		  } 
		  else 
		  {
			  pps_len += buff[0];
		  }
	  }
	  if(offset >= pps_start_offset && offset <= pps_start_offset + pps_len - 1)
	  {
		  pps[pps_len_start + startCodeNum] = buff[0];
		  pps_len_start++;
		  //printf("%x ",buff[0]);
	  }
	  offset++;

      if (len == 0)
         break;
   }
   
   int i;
   for(i  = 0 ; i < pps_len + startCodeNum ; i++)
   	  printf("%x ",pps[i]);


   stream_close(s);
   destory_file_stream(s);
   
   

	return 0;
}

