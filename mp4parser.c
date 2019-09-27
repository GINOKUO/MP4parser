#include "stdafx.h"
#include "mp4.h"
#include "malloc.h"
#include "string.h"


int main(int argc, char* argv[])
{
   int sampleCount;
   mp4_box_t *root = NULL, *stsz = NULL, *stco = NULL, *stsc = NULL;
   stream_t* fd = NULL;

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
   fd = create_buffer_stream();
   if (buffer_open(fd, buffer) == 0)
      return -1;

   root = MP4_BoxGetRootFromBuffer(fd,filesize);
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
   int chunk_offset_no = 0;
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
   buffer_close(fd);
   destory_buffer_stream(fd);
      
   ////////////////
   
   fd = create_file_stream();
   if (stream_open(fd, "test.mp4", MODE_READ) == 0)
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
      len = stream_read(fd, buff, 1);
	  buff_tmp[3] = buff_tmp[2];
	  buff_tmp[2] = buff_tmp[1];
	  buff_tmp[1] = buff_tmp[0];
	  buff_tmp[0] = buff[0];
	  if(buff_tmp[0] == avcC_type[0] && buff_tmp[1] == avcC_type[1] && buff_tmp[2] == avcC_type[2] && buff_tmp[3] == avcC_type[3])
	  {
		  avcC_start_offset = offset;
		  sps_len_offset[0] = avcC_start_offset + 7;
		  sps_len_offset[1] = sps_len_offset[0] + 1;
		  sps_start_offset = sps_len_offset[1] + 1;
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
		  sps[sps_len_start + startCodeNum] = buff[0]; // + startCodeNum
		  sps_len_start++;
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
		  pps[pps_len_start + startCodeNum] = buff[0]; // + startCodeNum
		  pps_len_start++;
	  }
	  
	  offset++;
	  
      if (len == 0)
         break;
   }

   stream_close(fd);
   destory_file_stream(fd);
   
   stream_t* h264 = NULL; 												//h264 file
   h264 = create_buf_file_stream(); 									//h264 file
   if (stream_open(h264, "test.h264", MODE_WRITE | MODE_CREATE) == 0)	//h264 file
      return -1;														//h264 file
   
   fd = create_file_stream();
   if (stream_open(fd, "test.mp4", MODE_READ) == 0)
      return -1;
  
   offset = 0;
   uint8_t *frame_buff = (uint8_t*)malloc(25000);
   int stsz_sample_count_num = 0;
   int sss_time = 0;
   int sco_count = 0;
   int chunk = 0;
   int frame_count = 0;
   
   uint8_t *spss_data = (uint8_t*)malloc(1024);	//h264 file
   uint8_t *ppss_data = (uint8_t*)malloc(1024);	//h264 file
   
   int i;
   for(i  = 0 ; i < sps_len + startCodeNum  ; i++) // + startCodeNum
   {
	   spss_data[i] = sps[i];                 //h264 file
   	   //printf("%x ",sps[i]);
   }
   for(i  = 0 ; i < pps_len + startCodeNum  ; i++)	// + startCodeNum
   {   	   
	  ppss_data[i] = pps[i];  				 //h264 file
	  //printf("%x ",pps[i]);
   }
   
   stream_write(h264, spss_data, sps_len + startCodeNum );   //h264 file + startCodeNum
   stream_write(h264, ppss_data, pps_len+ startCodeNum );   //h264 file + startCodeNum

   while(1)
   {
	  if(offset < stco_chunk_offset[sco_count])
	  {
		  stream_read(fd, frame_buff, 1);
	  	  offset++;
	  }
	  else
	  {
		 //printf("%d ",offset);
		 frame_count++;
		 stream_read(fd, frame_buff, stsz_smaple_size[stsz_sample_count_num]);
		 // -----------------> + startCodeNum
		 for(i = 0; i < 4; i++)
		 {
			if(i == 3)
				frame_buff[i] = 1;
			else
				frame_buff[i] = 0;
		 }
		 // <-----------------
		 offset += stsz_smaple_size[stsz_sample_count_num]; 
		 
		 stream_write(h264, frame_buff, stsz_smaple_size[stsz_sample_count_num]); //h264 file

		 /*
		 if(frame_count == 5130) {
		 int i;
		 printf("\nframe %d \n",frame_count);
         for(i  = 0 ; i < stsz_smaple_size[stsz_sample_count_num]  ; i++)
			printf("%x ",frame_buff[i]);
		 }
		 */
		 stsz_sample_count_num++;
		 chunk++;
		 if(sco_count == stsc_first_chunk[1] - 1) {
			if(chunk == stsc_samples_per_chunk[1] )
			{
				sco_count++;
				chunk = 0;
			}
		 } 
		 else
		 {
			if(chunk == stsc_samples_per_chunk[0] )
			{
				sco_count++;
				chunk = 0;
			}
		 }
	  }
	  
	  if (frame_count == stsz_sample_count )
         break;
	 

   }
   
   stream_close(fd);
   destory_file_stream(fd);
   
   stream_close(h264);			//h264 file
   destory_file_stream(h264);	//h264 file
  
	return 0;
}

