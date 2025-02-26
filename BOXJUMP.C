// TODO, in order from most important
// - allow for one more sine step in the jump, rotate and screen shake
// - make sure this game is stable and runs exactly the same on all
// platforms
// - add different platform levels (colour changeable)
// - camera will pan up/down according to the platform level
// - add "fall" rotation, for when box falls from a platform (M_PI_2)
// - add a config.txt file, and store user adjustable variables in them
// - add more user adjustable variables, eg box, bg, spike, platform colour
// - draw eyes and mouth on box (colour changeable)
// - add "bubble" double jump
// - soundsssss!!!

// Box Jump Game, written by Misha Soup

// compile with djgpp, liballegro and libmath

// NOTE - this game assumes that the compiler runs in 32-bit protected mode
// Yes, I know that it's weird that the whole game is condensed to a single
// file. That's just my coding style. Get used to it.
// Optimised to run as fast as possible, to the extent of my coding skills:)
// Each line has a maximum width of 76, to fit in DOS's default 80-column 
// mode with some padding for window borders
// Designed on and for a Fujitsu ESPRIMO U9200 with Intel Core 2 Duo T5750
// @ 2.0GHz running FreeDOS, however this game should work on any 386 or
// better CPU with a minimum of 4MB RAM, you probably will need to tinker
// with the #define variables below to achieve a smooth performance.

// only uses four libraries :)
#include <allegro.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// below are variables that are user adjustable
// try changing them and seeing the results!
#define box_size (75.0) // box size in pixels
#define max_rotate (M_PI) // maximum degree of rotation per jump(in radians)
#define max_jump_height (150.0) // jump height in pixels
#define jump_speed (0.5) // how many seconds it takes to complete a jump
#define tick_frequency (1000) // number of ticks per second
#define screen_width (800) // screen width in pixels
#define screen_height (600) // screen height in pixels
#define screen_bits (8) // bits per pixel
#define show_fps (1) // show fps. 0 = false, 1 = true
#define vsync_on (0) // turn on vsync. 0 = false, 1 = true
#define box_speed (750.0) // how fast the box moves, this affects obstacles
#define obstacle_size (50.0) // width of spikes, platforms and bubbles
#define platform_height (50.0) //height of platforms
#define platform_distance (100.0) // the y distance between platforms
#define shake_screen_freq (10) // shake screen frequency
#define screen_shake_speed (0.5) // how many seconds to shake screen for
#define screen_shake_mag (50.0) // strength of shake
#define lock_fps (0) // lock fps to variable below
#define fps (60.0) // maximum fps to render at if lock_fps == 1

// the variables below are used internally
// DO NOT MODIFY OR THE GAME MIGHT CRASH!
double box_vertices[8];
int box_vertices_int[8];
int is_jumping = 0;
double frame_delta = 0.0;
volatile int tick_count = 0;
double last_jump_step = 0.0;
double jump_count = 0.0;
double curr_rotate = 0.0;
double box_center_x;
double box_center_y;
double box_min_x, box_min_y;
double box_max_x, box_max_y;
double* obstacles;
int* obstacle_type;
int max_obstacles;
int next_obstacle = 0;
int last_obstacle = 0;
int obstacle_count = 0;
BITMAP* buffer;
double curr_fps = 0.0;
int frame_count = 0;
int last_tick = 0;
int is_dropping = 0;
int curr_platform = 0;
int next_platform = 0;
int is_running = 1;
int is_paused = 0;
int cam_is_moving = 0;
const double half_obst_size = (obstacle_size / 2.0);
int is_collided = 0;
int is_shaking = 0;
int do_reset_game = 0;
double last_shake_step = 0.0;
double screen_shake_count = 0.0;
const double max_delta_frame = (1.0 / fps);
volatile int lock_fps_tick = 0;

// allegro timer interurpt
static inline void tick_interrupt(void)
{
    tick_count++;
	
    return;
}
END_OF_FUNCTION(tick_interrupt);

// if lock_fps is 1, then we wait until this interrupt is called before
// drawing next frame
static inline void lock_fps_interrupt(void)
{
    lock_fps_tick = 1;

    return;
}
END_OF_FUNCTION(lock_fps_interrupt);

// default starting position
static inline void reset_box(void)
{
    box_vertices[0] = (double)(screen_width / 4) - (box_size / 2.0);
    box_vertices[1] = (double)(screen_height / 2) - (box_size / 2.0);
    box_vertices[2] = (double)(screen_width / 4) - (box_size / 2.0);
    box_vertices[3] = (double)(screen_height / 2) + (box_size / 2.0);
    box_vertices[4] = (double)(screen_width / 4) + (box_size / 2.0);
    box_vertices[5] = (double)(screen_height / 2) + (box_size / 2.0);
    box_vertices[6] = (double)(screen_width / 4) + (box_size / 2.0);
    box_vertices[7] = (double)(screen_height / 2) - (box_size / 2.0);
	
	box_min_x = box_vertices[0];
	box_min_y = box_vertices[1];
	box_max_x = box_vertices[4]; 
	box_max_y = box_vertices[5];
	
	box_center_x = box_min_x + ((box_max_x - box_min_x) / 2.0);
    box_center_y = box_min_y + ((box_max_y - box_min_y) / 2.0);
	
    return;
}

// rotates the box back to 0 around its center
static inline void reset_box_rotation(void)
{
    box_vertices[0] = box_center_x - (box_size / 2.0);
    box_vertices[1] = box_center_y - (box_size / 2.0);
    box_vertices[2] = box_center_x + (box_size / 2.0);
    box_vertices[3] = box_center_y - (box_size / 2.0);
    box_vertices[4] = box_center_x + (box_size / 2.0);
    box_vertices[5] = box_center_y + (box_size / 2.0);
    box_vertices[6] = box_center_x - (box_size / 2.0);
    box_vertices[7] = box_center_y + (box_size / 2.0);

    return;
}

// returns the minimum or maximum x or y vertice
static inline double get_vertice(int x_or_y, int min_or_max)
{
    double result = box_vertices[x_or_y];

    int i;

    for(i = (x_or_y + 2); i < 8; i += 2)
    {
		if(!min_or_max)
		{
			if(box_vertices[i] < result)
			{
				result = box_vertices[i];
			}
		}

		else
		{
			if(box_vertices[i] > result)
			{
				result = box_vertices[i];
			}
		}
    }

    return result;
}

// rotate box 
static inline void rotate_box(void)
{
    // reset the box's rotation to 0 before applying curr_rotate
    reset_box_rotation();

    double c = cos(curr_rotate);
    double s = sin(curr_rotate);
    double box_x, box_y;
    int i;

    for(i = 0; i < 8; i+= 2)
    {
		box_x = box_vertices[i];
		box_y = box_vertices[i + 1];

		box_vertices[i] = box_center_x + (c * (box_x - box_center_x)) -
			(s * (box_y - box_center_y));

		box_vertices[i + 1] = box_center_y + (s * (box_x - box_center_x)) +
			(c * (box_y - box_center_y)); 
    }

    return;
}

static inline void jump(void)
{
    double next_sine_step = 0.0;

    // get maximum x and y vertices
    box_min_x = get_vertice(0, 0);
    box_min_y = get_vertice(1, 0);
    box_max_x = get_vertice(0, 1);
    box_max_y = get_vertice(1, 1);

    // find center of box
    box_center_x = box_min_x + ((box_max_x - box_min_x) / 2.0);
    box_center_y = box_min_y + ((box_max_y - box_min_y) / 2.0);

    // increment total rotation by the frame delta
    curr_rotate += (max_rotate * frame_delta) / jump_speed;

    rotate_box();
	
	// update delta counter
    jump_count += frame_delta;

    // change box's y coords according to the sine wave
    next_sine_step = sinf((jump_count * M_PI) / jump_speed) * 
		max_jump_height;

    int i;

    for(i = 1; i < 8; i+= 2)
    {
		box_vertices[i] -= (next_sine_step - last_jump_step);
    }

    // save last sine step to be used for the next loop plus frame delta
    last_jump_step = next_sine_step;

    // check if we have jumped for jump_speed seconds
    if(curr_rotate > max_rotate)
	{
		is_jumping = 0;
		jump_count = 0.0;
		last_jump_step = 0.0;
		curr_rotate = 0.0;
		reset_box();
    }

    return;
}

// required to draw a polygon with allegro
static inline void cast_to_int(void)
{
    int i;

    for(i = 0; i < 8; i++)
    {
		box_vertices_int[i] = (int)(box_vertices[i]);
    }

    return;
}

// appends a spike to the spikes* array
static inline void add_spike(void)
{
    obstacles[next_obstacle] = (double)screen_width + half_obst_size;
    // will be modified when we add platforms
    obstacles[next_obstacle + 1] = (double)(screen_height / 2) + 
		(box_size / 2.0) - (next_platform * platform_distance);
	
    obstacle_type[next_obstacle / 2] = 1;
	
    // loop back to 0 if we have reached the end of the array
    if((next_obstacle + 2) == max_obstacles)
    {
		next_obstacle = 0;
    }

    else
    {
		next_obstacle += 2;
    }

    return;
}

// set up all global variables for the game
static inline void init_game(void)
{
	// if unable to initialize allegro or graphics, abort
    if(allegro_init() != 0)
	{
		abort();
	}
	
	set_color_depth(screen_bits);
	
	if(set_gfx_mode(GFX_AUTODETECT, screen_width, screen_height, 0, 0) != 0)
	{
		abort();
	}
    
	install_keyboard();
	install_timer();

    buffer = create_bitmap(SCREEN_W, SCREEN_H);

    LOCK_VARIABLE(tick_count);
    LOCK_FUNCTION(tick_interrupt);

    install_int_ex(tick_interrupt, BPS_TO_TIMER(tick_frequency));
	
	
    LOCK_VARIABLE(lock_fps_tick);
    LOCK_FUNCTION(lock_fps_interrupt);

    // if lock_fps = 1, set this interrupt
	if(lock_fps)
	{
		install_int_ex(lock_fps_interrupt, BPS_TO_TIMER((int)fps));
	}

    srand(time(0));
	
	reset_box();

    // allocate memory for spikes
    // maximum amount of spikes = screen_width / spike_size
    max_obstacles = (screen_width / (int)obstacle_size) * 2;
    obstacles = malloc(sizeof(double) * max_obstacles);
    obstacle_type = malloc(max_obstacles / 2);
	
    return;
}

// move all spikes
static inline void update_obstacles(void)
{
    int i = 0;

    for(i = last_obstacle; i != next_obstacle; i += 2)
    {
		// last spike is invisible, so we 'delete' it from the array
		if(obstacles[i] < -half_obst_size)
		{
			if((last_obstacle + 2) == max_obstacles)
			{
				last_obstacle = 0;
				obstacle_count--;
			}

			else
			{
				last_obstacle += 2;
				obstacle_count--;
			}
		}

		// move the spike
		else
		{
			obstacles[i] -= frame_delta * box_speed;
		}

		if((i + 2) == max_obstacles)
		{
			i = -2;
		}
    }

    return;
}

static inline void drop(void)
{
    // to be added after we add platforms

    return;
}

// add platform above or below the cube
static inline void add_platform(int up_down)
{
	if(up_down == 1)
	{
		next_platform++;
	}
	else
	{
		next_platform--;
	}
	
	obstacles[next_obstacle] = (double)screen_width + half_obst_size;
	
	obstacles[next_obstacle + 1] = (double)(screen_height / 2) +
		(box_size / 2.0) - (next_platform * platform_distance);
	
    obstacle_type[next_obstacle / 2] = 2;
	
    // loop back to 0 if we have reached the end of the array
    if((next_obstacle + 2) == max_obstacles)
    {
		next_obstacle = 0;
    }

    else
    {
		next_obstacle += 2;
    }
	
    return;
}

// move all obstacles - spikes, platforms, double jump bubbles etc
static inline void update_game(void)
{
	int newest_obstacle = -1;
	int can_add_obstacle = 0;
	
    // update box vectors if jumping or dropping
    if(is_jumping)
    {
		jump();
    }

    else if(is_dropping)
    {
		drop();
    }
	
	// if there are no obstacles, all is good to add
	if(obstacle_count == 0)
	{
		can_add_obstacle = 1;
	}
	
	// need to check whether to loop around the obstacles array
	else if(next_obstacle == 0)
	{
		newest_obstacle = max_obstacles - 2;
	}
	
	else
	{
		newest_obstacle = next_obstacle - 2;
	}
	
	
	if(newest_obstacle > -1)
	{
		// check if the most recently added obstacle has enough space for
		// box to land, to be updated
		if(obstacles[newest_obstacle] < (screen_width -
			half_obst_size - (box_size * 4.5)))
		{
			can_add_obstacle = 1;
		}
	}
	
	if(can_add_obstacle)
	{		
		// check if we need to add obstacles
		switch(rand() % 4)
		{
			case 1:
				add_spike();
				obstacle_count++;
				break;

			case 2:
				add_platform(1);
				obstacle_count++;
				break;

			case 3:
				if(curr_platform > 0)
				{
					add_platform(0);
					obstacle_count++;
				}
		}
	}
	
    update_obstacles();
	
    return;
}

static inline void draw_spikes(void)
{
    int spike_vertices[6];

    int i;

    // draw each spike
    for(i = last_obstacle; i != next_obstacle; i += 2)
    {
		if(obstacle_type[i / 2] != 1)
		{
			continue;
		}

		spike_vertices[0] = (int)(obstacles[i] - half_obst_size);
		spike_vertices[1] = (int)(obstacles[i + 1]);
		spike_vertices[2] = (int)(obstacles[i]);
		spike_vertices[3] = (int)(obstacles[i + 1] - (obstacle_size));
		spike_vertices[4] = (int)(obstacles[i] + half_obst_size);
		spike_vertices[5] = (int)(obstacles[i + 1]);

		polygon(buffer, 3, spike_vertices, makecol(0xFF, 0x00, 0x00));
		
		// loop back to start of array
		if((i + 2) == max_obstacles)
		{
			i = -2;
		}
    }

    return;
}

static inline void draw_platforms(void)
{
    int platform_vertices[8];

    int i;

    // draw each spike
    for(i = 0; i < max_obstacles; i += 2)
    {
		if(obstacle_type[i / 2] != 2)
		{
			continue;
		}

		platform_vertices[0] = (int)(obstacles[i] - half_obst_size);
		platform_vertices[1] = (int)(obstacles[i + 1]);
		platform_vertices[2] = (int)(obstacles[i]);
		platform_vertices[3] = (int)(obstacles[i + 1] - (obstacle_size));
		platform_vertices[4] = (int)(obstacles[i] + half_obst_size);
		platform_vertices[5] = (int)(obstacles[i + 1]);

		polygon(buffer, 4, platform_vertices, makecol(0xFF, 0x00, 0x00));
		
		// loop back to start of array
		if((i + 2) == max_obstacles)
		{
			i = -2;
		}
    }

    return;
}

// draw everything to buffer
static inline void draw(void)
{
    clear_to_color(buffer, makecol(0x00, 0x00, 0xFF));

    draw_spikes();
    draw_platforms();

    cast_to_int();
    polygon(buffer, 4, box_vertices_int, makecol(0x00, 0xFF, 0x00));

    if(show_fps)
    {
		textprintf_ex(buffer, font, 0, 0, makecol(0, 0, 0), -1, 
			"FPS - %.5f", curr_fps);
	}

    return;
}

// calculate fps
static inline void calc_fps(void)
{
    curr_fps = (((double)tick_frequency) * ((double)frame_count)) / 
		((double)tick_count);

    frame_delta = (double)(tick_count - last_tick) / (double)tick_frequency;

    if(tick_count > tick_frequency)
    {
		frame_count = 0;
		tick_count = 0;
    }

    last_tick = tick_count;

    return;
}

// check if point is on line segment
static inline int on_line_segment(double line_x1, double line_y1,
	double line_x2, double line_y2, double point_x, double point_y)
{
	double max_1 = (line_x1 > point_x) ? line_x1 : point_x;
	double min_1 = (line_x1 < point_x) ? line_x1 : point_x;
	double max_2 = (line_y1 > point_y) ? line_y1 : point_y;
	double min_2 = (line_y1 < point_y) ? line_y1 : point_y;
	
	if(line_x2 <= max_1 && line_x2 >= min_1 && line_y2 <= max_2 && 
		line_y2 >= min_2)
	{
		return 1;
	}
	
    return 0;
}

// find orientation of lines
static inline int line_orient(double x1, double y1, double x2, double y2,
	double x3, double y3) 
{
    int val = (int)((y2 - y1) * (x3 - x2) - (x2 - x1) * (y3 - y2));
	
    if (val == 0) return 0;
	
    return (val > 0) ? 1 : 2;
}

// check if two line segments intersect
static inline int line_intersect(double line_1_x1, double line_1_y1,
	double line_1_x2, double line_1_y2, double line_2_x1, double line_2_y1, 
	double line_2_x2, double line_2_y2) 
{
    int orient_1 = line_orient(line_1_x1, line_1_y1, line_1_x2, line_1_y2,
		line_2_x1, line_2_y1);
	
    int orient_2 = line_orient(line_1_x1, line_1_y1, line_1_x2, line_1_y2,
		line_2_x2, line_2_y2);
	
    int orient_3 = line_orient(line_2_x1, line_2_y1, line_2_x2, line_2_y2,
		line_1_x1, line_1_y1);
	
    int orient_4 = line_orient(line_2_x1, line_2_y1, line_2_x2, line_2_y2,
		line_1_x2, line_1_y2);
	
    if (orient_1 != orient_2 && orient_3 != orient_4)
	{
		return 1;
	}
	
	if (orient_1 == 0 && on_line_segment(line_1_x1, line_1_y1, line_2_x1,
		line_2_y1, line_1_x2, line_1_y2))
	{
		return 1;
	}
	
	if (orient_2 == 0 && on_line_segment(line_1_x1, line_1_y1, line_2_x2,
		line_2_y2, line_1_x2, line_1_y2))
	{
		return 1;
	}
	
	if (orient_3 == 0 && on_line_segment(line_2_x1, line_2_y1, line_1_x1,
		line_1_y1, line_2_x2, line_2_y2))
	{
		return 1;
	}
	
	if (orient_4 == 0 && on_line_segment(line_2_x1, line_2_y1, line_1_x2,
		line_1_y2, line_2_x2, line_2_y2))
	{
		return 1;
	}
  
    return 0;
}

// all function definitions were stolen (and modified) from
// https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/
// check if box is touching spike using line intersection testing
static inline int box_touch_spike(int close_spike)
{
	int i;
	
	for(i = 0; i < 8; i += 2)
	{
		// check if any of the box's edges are intersecting with the spike
		// need to check two lines of the spike triangle
		// (bottom one doesn't matter)
		if(line_intersect(box_vertices[i], box_vertices[i + 1], 
			box_vertices[((i + 2) % 8)], box_vertices[((i + 3) % 8)],
			(obstacles[close_spike] - half_obst_size),
			obstacles[close_spike + 1], obstacles[close_spike],
			(obstacles[close_spike + 1] - obstacle_size)))
		{
			return 1;
		}
		
		if(line_intersect(box_vertices[i], box_vertices[i + 1], 
			box_vertices[((i + 2) % 8)], box_vertices[((i + 3) % 8)],
			(obstacles[close_spike] + half_obst_size),
			obstacles[close_spike + 1], obstacles[close_spike],
			(obstacles[close_spike + 1] - obstacle_size)))
		{
			return 1;
		}
	}
	
    return 0;
}

// box-objects collision detection
static inline void detect_collision(void)
{
	int i;
	int is_close = 0;
	
	double min_obst_x, max_obst_x, min_obst_y;
	
	for(i = last_obstacle; i != next_obstacle; i += 2)
	{
		if(obstacle_type[i / 2] == 1)
		{
			min_obst_x = obstacles[i] - half_obst_size;
			max_obst_x = obstacles[i] + half_obst_size;
			min_obst_y = obstacles[i + 1] - obstacle_size;
			
			// use a fast detection before we get to the actual heavy
			// traingle collision detection
			if(min_obst_x > box_max_x)
			{
				is_close = 0;
			}
			
			else if(max_obst_x < box_min_x)
			{
				is_close = 0;
			}
			
			else if(min_obst_y > box_max_y)
			{
				is_close = 0;
			}
			
			else
			{
				is_close = 1;
			}
			
			if(is_close)
			{
				if(!is_jumping)
				{
					is_collided = 1;
					is_shaking = 1;
					break;
				}
				
				else
				{
					if(box_touch_spike(i))
					{
						is_collided = 1;
						is_shaking = 1;
						break;
					}
				}
			}
		}
		
		if((i + 2) == max_obstacles)
		{
			i = -2;
		}
	}
	
	return;
}

// check keyboard for input and update global variables accordingly
static inline void check_keyboard(void)
{
	if(keyboard_needs_poll())
	{
		poll_keyboard();
	}

	if(key[KEY_SPACE])
	{
		if(!is_jumping)
		{
			is_jumping = 1;
		}
	}

	if(key[KEY_ESC])
	{
		if(!is_paused)
		{
			is_paused = 1;
		}
	}

	if(key[KEY_ENTER])
	{
		if(is_paused)
		{
			is_paused = 0;
		}
	}
	
	if(key[KEY_R])
	{
		do_reset_game = 1;
	}

	if(key[KEY_Q])
	{
		is_running = 0;
	}
	
	return;
}

// shake the screen (ie, move all objects on the screen) using damped 
// sine wave
static inline void shake_screen(void)
{
	int i;
	
	// calculate amplitude damping with exponent
	float damped_amplitude = screen_shake_mag * exp((-4.0 /
		screen_shake_speed) * screen_shake_count);
	
	// how much to move each vertice
	double next_x = damped_amplitude * sin(2 * M_PI * shake_screen_freq *
		screen_shake_count);
	
	// keep track of last step and get difference
	double move_x = next_x - last_shake_step;
	
	// now update all obstacles and box
	for(i = 0; i < 8; i += 2)
	{
		box_vertices[i] -= move_x; 
	}
	
	for(i = last_obstacle; i != next_obstacle; i += 2)
	{
		obstacles[i] -= move_x;
		
		if((i + 2) == max_obstacles)
		{
			i = -2;
		}
	}
	
	screen_shake_count += frame_delta;
	
	if(screen_shake_count > screen_shake_speed)
	{
		is_shaking = 0;
	}
	
	last_shake_step = next_x;
	
	return;
}

// reset game to beginning
static inline void reset_game(void)
{
	is_jumping = 0;
	is_dropping = 0;
	last_jump_step = 0.0;
	jump_count = 0.0;
	curr_rotate = 0.0;
	reset_box();
	
	next_obstacle = 0;
	last_obstacle = 0;
	obstacle_count = 0;
	curr_platform = 0;
	cam_is_moving = 0;
	
	is_collided = 0;
	is_paused = 0;
	do_reset_game = 0;
	
	is_shaking = 0;
	last_shake_step = 0.0;
	screen_shake_count = 0.0;
	
	return;
}

// game loop function
static inline void run_game(void)
{
    while(is_running)
    {
		// first, check for keyboard input
		check_keyboard();
		
		if(do_reset_game)
		{
			reset_game();
		}
		
		if(is_paused)
		{
			// to do: draw pause menu and options
		}
		
		// if collided, then only shake screen, stop updating the game
		// to reset the game, press 'r'
		else if(is_collided)
		{
			if(is_shaking)
			{
				shake_screen();
			}
		}
		
		// game is running normally
		else
		{	
			// move box and everything else
			update_game();
			
			// detect for collision
			detect_collision();
		}
		
		// draw everything to buffer
		draw();
		
		// wait for vsync if needed
		if(vsync_on)
		{
			vsync();
		}

		// write buffer to video memory
		blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
		
		// not sure whether to move this to before or after vsync()??
		// lock frame rate by checking tick_count against tick_frequency
		if(lock_fps)
		{
			// wait for lock_fps_tick to become 1
			while(!lock_fps_tick){}
			lock_fps_tick = 0;
		}
		
		// update fps counter
		frame_count++;

		// calculate the frame per second and delta
		calc_fps();
    }
}

static inline void del_game(void)
{
	free(obstacles);
	free(obstacle_type);
	
	allegro_exit();
	
	return;
}

int main(int argc, char** argv)
{
    init_game();

    run_game();
	
	del_game();

    return 0;
}
