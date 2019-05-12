# CheckMp4Files

A tool to check mp4 video file compatibility with H264Dxva2Decoder.


USE_DIRECTORY 1 : check all files from directory (INPUT_DIRECTORY)

USE_DIRECTORY 0 : check one file (INPUT_FILE)

COUNT_TO_START_PARSE 0 to n : begin check at count when USE_DIRECTORY 1

PARSE_NALUNIT 1 : parse all video nalu from file

PARSE_NALUNIT 0 : do not parse nalu, just mp4 atom

CREATE_RAW_H264_DATA 1 : extract video stream from first video track (OUTPUT_FILE). Generate .h264 file
