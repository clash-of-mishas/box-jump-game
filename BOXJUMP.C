// TODO
// - use frame delta to cap game speed
// - change jump speed from pixels to seconds
// - add option to show fps ingame
// - change jump from linear to sine

// for window creation, rendering etc
#include <SDL2/SDL.h>
// required to add images
#include <SDL2/SDL_image.h>

// the width and height of the window is not const, because they can change on
// android
int window_width = 1920;
int window_height = 1080;

SDL_Window* game_window;
SDL_Renderer* game_renderer;

// conatant values, try changing them and seeing the results!
const int max_jump_height = 300;
const double jump_speed = 0.5; // length in seconds it takes to complete a jump
const double max_rotate = 180.0;

int is_running;
int is_jumping;
int is_falling;
int box_rest_y;
double box_x;
double box_y;
double rotate_step;
int heldDown;

double frame_delta;
const int vsync_on = 1;

const char* box_img_path = "images/box.png";
SDL_Texture* box_img;
SDL_Rect box_rect;

const char* background_img_path = "images/background.png";
SDL_Texture* background_img;

// initialise game variables
static inline void reset_game(void)
{
	is_running = 1;
	is_jumping = 0;
	is_falling = 0;
	
	box_rect.w = window_width / 10;
	box_rect.h = window_width / 10;
	box_rect.x = (window_width / 2) - (box_rect.w / 2);
	box_rect.y = (window_height / 2) - (box_rect.w / 2);
	
	box_x = (double)box_rect.x;
	box_y = (double)box_rect.y;
	
	box_rest_y = box_rect.y;
	
	rotate_step = 0.0;
	
	heldDown = 0;
	
	return;
}

// init window and renderer
static inline int init_game(void)
{
	// init all of SDL services
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return 1; // error
	}
	
	// init window
	game_window = SDL_CreateWindow("Box Jump Game", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, window_width, window_height,
		SDL_WINDOW_FULLSCREEN);
	
	// window creation failed
	if(game_window == 0)
	{
		return 1;
	}
	
	// update the window width and height with the actual values,
	// because on android it's subject to change
	SDL_GetWindowSize(game_window, &window_width, &window_height);
	
	if(vsync_on)
	{
		// init renderer
		game_renderer = SDL_CreateRenderer(game_window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	}
	
	else
	{
		// init renderer
		game_renderer = SDL_CreateRenderer(game_window, -1,
			SDL_RENDERER_ACCELERATED);
	}
	
	// renderer creation failed
	if(game_renderer == 0)
	{
		return 1;
	}
	
	// hide mouse cursor
	SDL_ShowCursor(SDL_DISABLE);
	
	reset_game();
	
	frame_delta = 0.0;
	
	return 0;
}

// load all the images
static inline int load_imgs(void)
{
	box_img = IMG_LoadTexture(game_renderer, box_img_path);
	
	// image load failed
	if(box_img == 0)
	{
		return 1;
	}
	
	background_img = IMG_LoadTexture(game_renderer, background_img_path);
	
	// image load failed
	if(background_img == 0)
	{
		return 1;
	}
	
	return 0;
}

// update game
static inline void update_game(void)
{
	if(is_jumping)
	{
		box_y -= (((double)max_jump_height * frame_delta) / jump_speed) * 2.0;
		rotate_step += ((max_rotate * frame_delta) / jump_speed);
		
		if(box_y < (box_rest_y - max_jump_height))
		{
			box_y = (box_rest_y - max_jump_height);
			is_jumping = 0;
			is_falling = 1;
		}
	}
	
	else if(is_falling)
	{
		box_y += (((double)max_jump_height * frame_delta) / jump_speed) * 2.0;
		rotate_step += ((max_rotate * frame_delta) / jump_speed);
		
		if(box_y > box_rest_y)
		{
			box_y = box_rest_y;
			is_falling = 0;
			// reset the rotation to default
			rotate_step = 0.0;
		}
	}
	
	return;
}

// game loop
static inline void run_game(void)
{
	double tick_speed = (double)SDL_GetPerformanceFrequency();
	Uint64 start_frame_tick, end_frame_tick;
	
	while(is_running)
    {
    	// get tick at start of frame
    	start_frame_tick = SDL_GetPerformanceCounter();
    	
    	// poll for events
    	SDL_Event e;
		if(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT)
			{
				break;
			}
			
			else if(e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE)
			{
				break;
			}
			
			else if(e.key.keysym.sym == SDLK_SPACE)
			{
				if(e.type == SDL_KEYDOWN)
				{
					if(heldDown == 0)
					{
						heldDown = 1;
					}
				}
				
				if(e.type == SDL_KEYUP)
				{
					if(heldDown == 1)
					{
						heldDown = 0;
					}
				}
			}
		}
		
		if(heldDown)
		{
			if(is_jumping == 0 && is_falling == 0)
			{
				is_jumping = 1;
			}
		}
		
		// update game
		update_game();
		
		// clear screen
		SDL_RenderClear(game_renderer);
		
		// draw background
		SDL_RenderCopy(game_renderer, background_img, 0, 0);
		
		// need to cast double to int for SDL to be able to draw it
		box_rect.x = (int)box_x;
		box_rect.y = (int)box_y;
		
		// copy box onto backbuffer
		SDL_RenderCopyEx(game_renderer, box_img, 0, &box_rect, rotate_step, 0, 0);
		
		// switch backbuffer to front
		SDL_RenderPresent(game_renderer);
		
		// get end frame tick
		end_frame_tick = SDL_GetPerformanceCounter();
		frame_delta = ((double)(end_frame_tick) - (double)(start_frame_tick)) / (double)tick_speed;
    }
}

// free all allocated memory
static inline void del_game(void)
{
	// will need to delete all images loaded
	SDL_DestroyTexture(box_img);
	SDL_DestroyTexture(background_img);
	
	SDL_DestroyRenderer(game_renderer);
	SDL_DestroyWindow(game_window);
	
	return;
}

int main(void)
{
	// If this function returns 0, all is good, otherwise quit
	if(init_game() != 0)
	{
		return 0;
	}
	
	if(load_imgs() != 0)
	{
		return 0;
	}
	
	run_game();
	
	del_game();
	
	return 0;
}
