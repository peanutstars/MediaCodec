## Test Code For Video/Still Image Clips

Simple Code for Testing Media Content and CODEC

#### MediaCode/HEVC/hevc_sei.c

- To get a SEI data in hevc.

#### MediaCode/FFavfilter/applyAVFilterToStillImage.c

Date : July 24 2015

- Processing to use FFMPEG Library, Just test.
  - Decoding Still Image
  - Convert pixel format from a decoded image to AV_PIX_FMT_YUV420P
  - Convert colormatrix from BT.601 to BT.709


- This code has some issue about memory leak.
  - Occuring memory leak when converting colormatrix.
  - Temporarily, changed code with fork, but it is too expensively.
