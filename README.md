# delogo
Video Delogo: Find the Logo Position and Size in a Video Stream v1.0 - Zelea

Usage: delogo [options] video_file
 - -?,--help             show options help
 - -i,--info             show video info only
 - -y,--canny            force canny edge detection for logo
 - -2,--two_rectangles   cover the logo with two overlaping rectangles
 - -m,--removelogo       generate a mask of the logo instead of a rectangle
 - -r,--pixel_ratio      switch to mask mode if pixel percentage inside the logo
                         bounding box is bellow (default 0%)
 - -a,--aggressive_crop  crop video aggressively
                         ignoring some percentage of non-black pixels
 - -c,--corner           fix logo corner [0-3] (NW,NE,SW,SE)
 - -j,--jump_corner      logo jumps from corner to corner
 - -f,--frames           number of frames to process (default 64)
 - -W,--width_bands      width percentage search band (default 35%)
 - -H,--height_bands     height percentage search band (default 18%)
 - -b,--level_black      maximum black level (default 23)
 - -d,--level_delta      maximum luminance distace for logo detection (default 65)
 - -k,--border_black     border thickness around logo (default 15)
 - -z,--border_band      additional border band (default 4)
 - -t,--threshold        canny edge detection threshold 0-255 (default 170)
 - -s,--sigma            canny sigma (default 1.2)
 - -l,--tlow             canny tlow (default 0.3)
 - -h,--thigh            canny thigh (default 0.8)
 - -X,--save_as_pcx      save images as PCX rather than PNG
