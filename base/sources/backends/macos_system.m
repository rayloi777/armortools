#import "macos_system.h"
#import <Cocoa/Cocoa.h>
#include <iron_gpu.h>
#include <iron_math.h>
#include <iron_system.h>
#include <iron_video.h>
#include <mach/mach_time.h>
#include <objc/runtime.h>
#include <stdbool.h>

struct WindowData {
	id   handle;
	id   view;
	bool fullscreen;
	void (*resizeCallback)(int width, int height, void *data);
	void *resizeCallbackData;
	bool (*closeCallback)(void *data);
	void *closeCallbackData;
};

static struct WindowData windows[1]            = {0};
static bool              controlKeyMouseButton = false;
static int               mouseX, mouseY;
static bool              keyboardShown  = false;
static const char       *videoFormats[] = {"ogv", NULL};
static NSApplication    *myapp;
static NSWindow         *window;
static BasicMTKView     *view;
static char              language[3];
static int               current_cursor_index = 0;

@implementation BasicMTKView

static bool shift = false;
static bool ctrl  = false;
static bool alt   = false;
static bool cmd   = false;

// Map macOS virtual key codes (hardware positions) to KEY_CODE enum values.
// Virtual key codes are layout-independent — kVK_ANSI_A is always the physical
// key at the A position regardless of QWERTY, AZERTY, Dvorak, etc.
static int macos_keycode_to_iron(unsigned short vk) {
	switch (vk) {
	// Letters (ANSI physical positions)
	case 0x00: return KEY_CODE_A;     // kVK_ANSI_A
	case 0x01: return KEY_CODE_S;     // kVK_ANSI_S
	case 0x02: return KEY_CODE_D;     // kVK_ANSI_D
	case 0x03: return KEY_CODE_F;     // kVK_ANSI_F
	case 0x04: return KEY_CODE_H;     // kVK_ANSI_H
	case 0x05: return KEY_CODE_G;     // kVK_ANSI_G
	case 0x06: return KEY_CODE_Z;     // kVK_ANSI_Z
	case 0x07: return KEY_CODE_X;     // kVK_ANSI_X
	case 0x08: return KEY_CODE_C;     // kVK_ANSI_C
	case 0x09: return KEY_CODE_V;     // kVK_ANSI_V
	case 0x0B: return KEY_CODE_B;     // kVK_ANSI_B
	case 0x0C: return KEY_CODE_Q;     // kVK_ANSI_Q
	case 0x0D: return KEY_CODE_W;     // kVK_ANSI_W
	case 0x0E: return KEY_CODE_E;     // kVK_ANSI_E
	case 0x0F: return KEY_CODE_R;     // kVK_ANSI_R
	case 0x10: return KEY_CODE_Y;     // kVK_ANSI_Y
	case 0x11: return KEY_CODE_T;     // kVK_ANSI_T
	case 0x1F: return KEY_CODE_O;     // kVK_ANSI_O
	case 0x20: return KEY_CODE_U;     // kVK_ANSI_U
	case 0x22: return KEY_CODE_I;     // kVK_ANSI_I
	case 0x23: return KEY_CODE_P;     // kVK_ANSI_P
	case 0x25: return KEY_CODE_L;     // kVK_ANSI_L
	case 0x26: return KEY_CODE_J;     // kVK_ANSI_J
	case 0x28: return KEY_CODE_K;     // kVK_ANSI_K
	case 0x2D: return KEY_CODE_N;     // kVK_ANSI_N
	case 0x2E: return KEY_CODE_M;     // kVK_ANSI_M

	// Number row (physical positions, not sequential in virtual key codes)
	case 0x12: return KEY_CODE_1;     // kVK_ANSI_1
	case 0x13: return KEY_CODE_2;     // kVK_ANSI_2
	case 0x14: return KEY_CODE_3;     // kVK_ANSI_3
	case 0x15: return KEY_CODE_4;     // kVK_ANSI_4
	case 0x17: return KEY_CODE_5;     // kVK_ANSI_5
	case 0x16: return KEY_CODE_6;     // kVK_ANSI_6
	case 0x1A: return KEY_CODE_7;     // kVK_ANSI_7
	case 0x1C: return KEY_CODE_8;     // kVK_ANSI_8
	case 0x19: return KEY_CODE_9;     // kVK_ANSI_9
	case 0x1D: return KEY_CODE_0;     // kVK_ANSI_0

	// Punctuation (physical positions)
	case 0x21: return KEY_CODE_OPEN_BRACKET;    // kVK_ANSI_LeftBracket
	case 0x1E: return KEY_CODE_CLOSE_BRACKET;   // kVK_ANSI_RightBracket
	case 0x29: return KEY_CODE_SEMICOLON;       // kVK_ANSI_Semicolon
	case 0x27: return KEY_CODE_QUOTE;           // kVK_ANSI_Quote
	case 0x2A: return KEY_CODE_BACK_SLASH;      // kVK_ANSI_Backslash
	case 0x2B: return KEY_CODE_COMMA;           // kVK_ANSI_Comma
	case 0x2F: return KEY_CODE_PERIOD;          // kVK_ANSI_Period
	case 0x2C: return KEY_CODE_SLASH;           // kVK_ANSI_Slash
	case 0x32: return KEY_CODE_BACK_QUOTE;      // kVK_ANSI_Grave
	case 0x1B: return KEY_CODE_HYPHEN_MINUS;    // kVK_ANSI_Minus
	case 0x18: return KEY_CODE_EQUALS;          // kVK_ANSI_Equal

	// Control keys
	case 0x24: return KEY_CODE_RETURN;          // kVK_Return
	case 0x30: return KEY_CODE_TAB;             // kVK_Tab
	case 0x31: return KEY_CODE_SPACE;           // kVK_Space
	case 0x33: return KEY_CODE_BACKSPACE;       // kVK_Delete (backspace)
	case 0x35: return KEY_CODE_ESCAPE;          // kVK_Escape
	case 0x75: return KEY_CODE_DELETE;          // kVK_ForwardDelete
	case 0x73: return KEY_CODE_HOME;            // kVK_Home
	case 0x77: return KEY_CODE_END;             // kVK_End
	case 0x74: return KEY_CODE_PAGE_UP;         // kVK_PageUp
	case 0x79: return KEY_CODE_PAGE_DOWN;       // kVK_PageDown

	// Modifiers
	case 0x38: return KEY_CODE_SHIFT;           // kVK_Shift
	case 0x3C: return KEY_CODE_SHIFT;           // kVK_RightShift
	case 0x3B: return KEY_CODE_CONTROL;         // kVK_Control
	case 0x3E: return KEY_CODE_CONTROL;         // kVK_RightControl
	case 0x3A: return KEY_CODE_ALT;             // kVK_Option
	case 0x3D: return KEY_CODE_ALT;             // kVK_RightOption
	case 0x37: return KEY_CODE_META;            // kVK_Command
	case 0x36: return KEY_CODE_META;            // kVK_RightCommand
	case 0x39: return KEY_CODE_CAPS_LOCK;       // kVK_CapsLock

	// Arrow keys
	case 0x7B: return KEY_CODE_LEFT;            // kVK_LeftArrow
	case 0x7C: return KEY_CODE_RIGHT;           // kVK_RightArrow
	case 0x7D: return KEY_CODE_DOWN;            // kVK_DownArrow
	case 0x7E: return KEY_CODE_UP;              // kVK_UpArrow

	// Function keys
	case 0x7A: return KEY_CODE_F1;
	case 0x78: return KEY_CODE_F2;
	case 0x63: return KEY_CODE_F3;
	case 0x76: return KEY_CODE_F4;
	case 0x60: return KEY_CODE_F5;
	case 0x61: return KEY_CODE_F6;
	case 0x62: return KEY_CODE_F7;
	case 0x64: return KEY_CODE_F8;
	case 0x65: return KEY_CODE_F9;
	case 0x6D: return KEY_CODE_F10;
	case 0x67: return KEY_CODE_F11;
	case 0x6F: return KEY_CODE_F12;
	case 0x69: return KEY_CODE_F13;
	case 0x6B: return KEY_CODE_F14;
	case 0x71: return KEY_CODE_F15;
	case 0x6A: return KEY_CODE_F16;
	case 0x40: return KEY_CODE_F17;
	case 0x4F: return KEY_CODE_F18;
	case 0x50: return KEY_CODE_F19;
	case 0x5A: return KEY_CODE_F20;

	// Numpad
	case 0x52: return KEY_CODE_NUMPAD_0;
	case 0x53: return KEY_CODE_NUMPAD_1;
	case 0x54: return KEY_CODE_NUMPAD_2;
	case 0x55: return KEY_CODE_NUMPAD_3;
	case 0x56: return KEY_CODE_NUMPAD_4;
	case 0x57: return KEY_CODE_NUMPAD_5;
	case 0x58: return KEY_CODE_NUMPAD_6;
	case 0x59: return KEY_CODE_NUMPAD_7;
	case 0x5B: return KEY_CODE_NUMPAD_8;
	case 0x5C: return KEY_CODE_NUMPAD_9;
	case 0x4B: return KEY_CODE_DIVIDE;          // kVK_ANSI_KeypadDivide
	case 0x4C: return KEY_CODE_RETURN;          // kVK_ANSI_KeypadEnter
	case 0x4E: return KEY_CODE_SUBTRACT;        // kVK_ANSI_KeypadMinus
	case 0x43: return KEY_CODE_MULTIPLY;        // kVK_ANSI_KeypadMultiply
	case 0x45: return KEY_CODE_ADD;             // kVK_ANSI_KeypadPlus
	case 0x41: return KEY_CODE_DECIMAL;         // kVK_ANSI_KeypadDecimal
	case 0x51: return KEY_CODE_EQUALS;          // kVK_ANSI_KeypadEquals
	case 0x47: return KEY_CODE_NUM_LOCK;        // kVK_ANSI_KeypadClear

	// Other
	case 0x72: return KEY_CODE_HELP;            // kVK_Help
	case 0x6E: return KEY_CODE_CONTEXT_MENU;    // kVK_ContextualMenu

	default: return KEY_CODE_UNKNOWN;
	}
}

- (void)flagsChanged:(NSEvent *)theEvent {
	unsigned short vk = [theEvent keyCode];
	unsigned long flags = [theEvent modifierFlags];

	switch (vk) {
	case 0x38: // kVK_Shift
	case 0x3C: // kVK_RightShift
		if (shift) {
			iron_internal_keyboard_trigger_key_up(KEY_CODE_SHIFT);
			shift = false;
		}
		if (flags & NSShiftKeyMask) {
			iron_internal_keyboard_trigger_key_down(KEY_CODE_SHIFT);
			shift = true;
		}
		break;
	case 0x3B: // kVK_Control
	case 0x3E: // kVK_RightControl
		if (ctrl) {
			iron_internal_keyboard_trigger_key_up(KEY_CODE_CONTROL);
			ctrl = false;
		}
		if (flags & NSControlKeyMask) {
			iron_internal_keyboard_trigger_key_down(KEY_CODE_CONTROL);
			ctrl = true;
		}
		break;
	case 0x3A: // kVK_Option
	case 0x3D: // kVK_RightOption
		if (alt) {
			iron_internal_keyboard_trigger_key_up(KEY_CODE_ALT);
			alt = false;
		}
		if (flags & NSAlternateKeyMask) {
			iron_internal_keyboard_trigger_key_down(KEY_CODE_ALT);
			alt = true;
		}
		break;
	case 0x37: // kVK_Command
	case 0x36: // kVK_RightCommand
		if (cmd) {
			iron_internal_keyboard_trigger_key_up(KEY_CODE_META);
			cmd = false;
		}
		if (flags & NSCommandKeyMask) {
			iron_internal_keyboard_trigger_key_down(KEY_CODE_META);
			cmd = true;
		}
		break;
	}
}

- (void)keyDown:(NSEvent *)theEvent {
	if ([theEvent isARepeat])
		return;

	unsigned short vk = [theEvent keyCode];
	int key_code = macos_keycode_to_iron(vk);

	if (key_code != KEY_CODE_UNKNOWN) {
		iron_internal_keyboard_trigger_key_down(key_code);
	}

	// Handle clipboard shortcuts via Command key
	if ([theEvent modifierFlags] & NSCommandKeyMask) {
		if (key_code == KEY_CODE_X) {
			char *text = iron_internal_cut_callback();
			if (text != NULL) {
				NSPasteboard *board = [NSPasteboard generalPasteboard];
				[board clearContents];
				[board setString:[NSString stringWithUTF8String:text] forType:NSStringPboardType];
			}
		}
		if (key_code == KEY_CODE_C) {
			char *text = iron_internal_copy_callback();
			if (text != NULL) {
				iron_copy_to_clipboard(text);
			}
		}
		if (key_code == KEY_CODE_V) {
			NSPasteboard *board = [NSPasteboard generalPasteboard];
			NSString     *data  = [board stringForType:NSStringPboardType];
			if (data != nil) {
				char charData[4096];
				strcpy(charData, [data UTF8String]);
				iron_internal_paste_callback(charData);
			}
		}
	}

	// Trigger key_press for text character input (layout-dependent)
	NSString *characters = [theEvent charactersIgnoringModifiers];
	if ([characters length]) {
		unichar ch = [characters characterAtIndex:0];
		if (vk == 0x24) {       // Return
			iron_internal_keyboard_trigger_key_press('\n');
		}
		else if (vk == 0x33) {  // Backspace
			iron_internal_keyboard_trigger_key_press('\x08');
		}
		else if (vk == 0x30) {  // Tab
			iron_internal_keyboard_trigger_key_press('\t');
		}
		else if (ch >= 32 && ch < 127) {
			iron_internal_keyboard_trigger_key_press(ch);
		}
	}
}

- (void)keyUp:(NSEvent *)theEvent {
	unsigned short vk = [theEvent keyCode];
	int key_code = macos_keycode_to_iron(vk);

	if (key_code != KEY_CODE_UNKNOWN) {
		iron_internal_keyboard_trigger_key_up(key_code);
	}
}

static int getMouseX(NSEvent *event) {
	NSWindow *window = [[NSApplication sharedApplication] mainWindow];
	float     scale  = [window backingScaleFactor];
	return (int)([event locationInWindow].x * scale);
}

static int getMouseY(NSEvent *event) {
	NSWindow *window = [[NSApplication sharedApplication] mainWindow];
	float     scale  = [window backingScaleFactor];
	return (int)(iron_window_height() - [event locationInWindow].y * scale);
}

- (void)mouseDown:(NSEvent *)theEvent {
	if ([theEvent modifierFlags] & NSControlKeyMask) {
		controlKeyMouseButton = true;
		iron_internal_mouse_trigger_press(1, getMouseX(theEvent), getMouseY(theEvent));
	}
	else {
		controlKeyMouseButton = false;
		iron_internal_mouse_trigger_press(0, getMouseX(theEvent), getMouseY(theEvent));
	}

	if ([theEvent subtype] == NSTabletPointEventSubtype) {
		iron_internal_pen_trigger_press(getMouseX(theEvent), getMouseY(theEvent), theEvent.pressure);
	}
}

- (void)mouseUp:(NSEvent *)theEvent {
	if (controlKeyMouseButton) {
		iron_internal_mouse_trigger_release(1, getMouseX(theEvent), getMouseY(theEvent));
	}
	else {
		iron_internal_mouse_trigger_release(0, getMouseX(theEvent), getMouseY(theEvent));
	}
	controlKeyMouseButton = false;

	if ([theEvent subtype] == NSTabletPointEventSubtype) {
		iron_internal_pen_trigger_release(getMouseX(theEvent), getMouseY(theEvent), theEvent.pressure);
	}
}

- (void)mouseMoved:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_move(getMouseX(theEvent), getMouseY(theEvent));
}

- (void)mouseDragged:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_move(getMouseX(theEvent), getMouseY(theEvent));

	if ([theEvent subtype] == NSTabletPointEventSubtype) {
		iron_internal_pen_trigger_move(getMouseX(theEvent), getMouseY(theEvent), theEvent.pressure);
	}
}

- (void)rightMouseDown:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_press(1, getMouseX(theEvent), getMouseY(theEvent));
}

- (void)rightMouseUp:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_release(1, getMouseX(theEvent), getMouseY(theEvent));
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_move(getMouseX(theEvent), getMouseY(theEvent));
}

- (void)otherMouseDown:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_press(2, getMouseX(theEvent), getMouseY(theEvent));
}

- (void)otherMouseUp:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_release(2, getMouseX(theEvent), getMouseY(theEvent));
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
	iron_internal_mouse_trigger_move(getMouseX(theEvent), getMouseY(theEvent));
}

- (void)scrollWheel:(NSEvent *)theEvent {
	float delta = [theEvent deltaY];
	iron_internal_mouse_trigger_scroll(-delta);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	NSPasteboard   *pboard         = [sender draggingPasteboard];
	NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];
	if ([[pboard types] containsObject:NSURLPboardType]) {
		if (sourceDragMask & NSDragOperationLink) {
			return NSDragOperationLink;
		}
	}
	return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	if ([[pboard types] containsObject:NSURLPboardType]) {
		NSArray *urls = [pboard readObjectsForClasses:@[ [NSURL class] ] options:nil];
		for (NSURL *fileURL in urls) {
			const char *filePath = [fileURL.path cStringUsingEncoding:NSUTF8StringEncoding];
			iron_internal_drop_files_callback(filePath);
		}
	}
	return YES;
}

- (id)initWithFrame:(NSRect)frameRect {
	self = [super initWithFrame:frameRect];

	device       = MTLCreateSystemDefaultDevice();
	commandQueue = [device newCommandQueue];
	library      = [device newDefaultLibrary];

	compositionX      = 0;
	compositionY      = 0;
	compositionHeight = 20;

	CAMetalLayer *metalLayer   = (CAMetalLayer *)self.layer;
	metalLayer.device          = device;
	metalLayer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
	metalLayer.framebufferOnly = YES;
	metalLayer.opaque          = YES;
	metalLayer.backgroundColor = nil;
	return self;
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)becomeFirstResponder {
	return YES;
}

- (BOOL)resignFirstResponder {
	return YES;
}

- (void)resize:(NSSize)size {
	[self setFrameSize:size];
}

- (CAMetalLayer *)metalLayer {
	return (CAMetalLayer *)self.layer;
}

- (id<MTLDevice>)metalDevice {
	return device;
}

- (id<MTLCommandQueue>)metalQueue {
	return commandQueue;
}

#pragma mark - NSTextInputClient Protocol

- (BOOL)hasMarkedText {
	return markedText != nil && markedText.length > 0;
}

- (NSRange)markedRange {
	return markedTextRange;
}

- (NSRange)selectedRange {
	return selectedTextRange;
}

- (NSArray *)validAttributesForMarkedText {
	return @[];
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selRange replacementRange:(NSRange)replRange {
	if ([string isKindOfClass:[NSAttributedString class]]) {
		markedText = [string string];
	} else {
		markedText = string;
	}
	markedTextRange = NSMakeRange(0, markedText.length);
	selectedTextRange = selRange;

	iron_internal_ime_composition_updated([markedText UTF8String], (int)selRange.location);
}

- (void)insertText:(id)string replacementRange:(NSRange)replRange {
	NSString *text;
	if ([string isKindOfClass:[NSAttributedString class]]) {
		text = [string string];
	} else {
		text = string;
	}

	iron_internal_ime_text_committed([text UTF8String]);

	markedText = nil;
	markedTextRange = NSMakeRange(NSNotFound, 0);
	selectedTextRange = NSMakeRange(0, 0);
}

- (void)unmarkText {
	markedText = nil;
	markedTextRange = NSMakeRange(NSNotFound, 0);
	selectedTextRange = NSMakeRange(0, 0);

	iron_internal_ime_composition_updated("", 0);
}

- (NSAttributedString *)attributedSubstringFromRange:(NSRange)range {
	if (markedText && range.location + range.length <= markedText.length) {
		return [[NSAttributedString alloc] initWithString:[markedText substringWithRange:range]];
	}
	return nil;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
	if (actualRange != NULL) {
		*actualRange = range;
	}
	NSWindow *win = [self window];
	if (win) {
		NSRect frame = [win frame];
		float  scale = [win backingScaleFactor];
		float  screenX = frame.origin.x + compositionX / scale;
		float  screenY = frame.origin.y + frame.size.height - compositionY / scale;
		return NSMakeRect(screenX, screenY - compositionHeight / scale, 0, compositionHeight / scale);
	}
	return NSMakeRect(compositionX, compositionY, 0, compositionHeight);
}

- (NSRect)firstRectForCharacterRange:(NSRange)range {
	return [self firstRectForCharacterRange:range actualRange:NULL];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
	return 0;
}

- (void)setCompositionPosition:(float)x y:(float)y height:(float)height {
	compositionX      = x;
	compositionY      = y;
	compositionHeight = height;
}

- (void)doCommandBySelector:(SEL)selector {
	if (selector == @selector(insertNewline:)) {
		iron_internal_keyboard_trigger_key_down(KEY_CODE_RETURN);
		iron_internal_keyboard_trigger_key_press('\n');
	} else if (selector == @selector(deleteBackward:)) {
		if ([self hasMarkedText]) {
			return;
		}
		iron_internal_keyboard_trigger_key_down(KEY_CODE_BACKSPACE);
		iron_internal_keyboard_trigger_key_press('\x08');
	} else if (selector == @selector(deleteForward:)) {
		if ([self hasMarkedText]) {
			return;
		}
		iron_internal_keyboard_trigger_key_down(KEY_CODE_DELETE);
		iron_internal_keyboard_trigger_key_press('\x7F');
	} else if (selector == @selector(moveLeft:)) {
		iron_internal_keyboard_trigger_key_down(KEY_CODE_LEFT);
	} else if (selector == @selector(moveRight:)) {
		iron_internal_keyboard_trigger_key_down(KEY_CODE_RIGHT);
	} else if (selector == @selector(moveUp:)) {
		iron_internal_keyboard_trigger_key_down(KEY_CODE_UP);
	} else if (selector == @selector(moveDown:)) {
		iron_internal_keyboard_trigger_key_down(KEY_CODE_DOWN);
	} else if (selector == @selector(cancelOperation:)) {
		[self unmarkText];
	}
}

@end

void iron_macos_set_ime_position(float x, float y, float height) {
	[view setCompositionPosition:x y:y height:height];
}

void iron_copy_to_clipboard(const char *text) {
	NSPasteboard *board = [NSPasteboard generalPasteboard];
	[board clearContents];
	[board setString:[NSString stringWithUTF8String:text] forType:NSStringPboardType];
}

int iron_count_displays(void) {
	NSArray *screens = [NSScreen screens];
	return (int)[screens count];
}

int iron_primary_display(void) {
	NSArray  *screens      = [NSScreen screens];
	NSScreen *mainScreen   = [NSScreen mainScreen];
	int       max_displays = 8;
	for (int i = 0; i < max_displays; ++i) {
		if (mainScreen == screens[i]) {
			return i;
		}
	}
	return -1;
}

void iron_display_init(void) {}

iron_display_mode_t iron_display_current_mode(int display) {
	NSArray            *screens    = [NSScreen screens];
	NSScreen           *screen     = screens[display];
	NSRect              screenRect = [screen frame];
	iron_display_mode_t dm;
	dm.width          = screenRect.size.width;
	dm.height         = screenRect.size.height;
	dm.frequency      = 60;
	dm.bits_per_pixel = 32;

	NSDictionary *description         = [screen deviceDescription];
	NSSize        displayPixelSize    = [[description objectForKey:NSDeviceSize] sizeValue];
	NSNumber     *screenNumber        = [description objectForKey:@"NSScreenNumber"];
	CGSize        displayPhysicalSize = CGDisplayScreenSize([screenNumber unsignedIntValue]);            // in millimeters
	double        ppi                 = displayPixelSize.width / (displayPhysicalSize.width * 0.039370); // Convert MM to INCH
	dm.pixels_per_inch                = round(ppi);

	return dm;
}

bool iron_mouse_can_lock(void) {
	return true;
}

void iron_mouse_show(void) {
	CGDisplayShowCursor(kCGDirectMainDisplay);
}

void iron_mouse_hide(void) {
	CGDisplayHideCursor(kCGDirectMainDisplay);
}

void iron_mouse_set_position(int x, int y) {
	NSWindow *window      = windows[0].handle;
	float     scale       = [window backingScaleFactor];
	NSRect    rect        = [[window contentView] bounds];
	NSPoint   windowpoint = NSMakePoint(x / scale, rect.size.height - y / scale);
	NSPoint   screenpoint = [window convertPointToScreen:windowpoint];
	CGPoint   cgpoint     = CGPointMake(screenpoint.x, [[NSScreen mainScreen] frame].size.height - screenpoint.y);
	CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, cgpoint);
	CGAssociateMouseAndMouseCursorPosition(true);
}

void iron_mouse_get_position(int *x, int *y) {
	NSWindow *window = windows[0].handle;
	float     scale  = [window backingScaleFactor];
	NSRect    rect   = [[window contentView] bounds];
	NSPoint   point  = [window mouseLocationOutsideOfEventStream];
	*x               = (int)(point.x * scale);
	*y               = (int)((rect.size.height - point.y) * scale);
}

void iron_mouse_set_cursor(iron_cursor_t cursor_index) {
	if (current_cursor_index == cursor_index) {
		return;
	}
	current_cursor_index = cursor_index;
	if (cursor_index == IRON_CURSOR_HAND) {
		[[NSCursor pointingHandCursor] set];
	}
	else if (cursor_index == IRON_CURSOR_IBEAM) {
		[[NSCursor IBeamCursor] set];
	}
	else if (cursor_index == IRON_CURSOR_SIZEWE) {
		[[NSCursor resizeLeftRightCursor] set];
	}
	else if (cursor_index == IRON_CURSOR_SIZENS) {
		[[NSCursor resizeUpDownCursor] set];
	}
	else {
		[[NSCursor arrowCursor] set];
	}
}

void iron_keyboard_show(void) {
	keyboardShown = true;
}

void iron_keyboard_hide(void) {
	keyboardShown = false;
}

bool iron_keyboard_active(void) {
	return keyboardShown;
}

const char *iron_system_id(void) {
	return "macOS";
}

const char **iron_video_formats(void) {
	return videoFormats;
}

void iron_set_keep_screen_on(bool on) {}

double iron_frequency(void) {
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	return (double)info.denom / (double)info.numer / 1e-9;
}

uint64_t iron_timestamp(void) {
	return mach_absolute_time();
}

bool with_autoreleasepool(bool (*f)(void)) {
	@autoreleasepool {
		return f();
	}
}

const char *iron_get_resource_path(void) {
	return [[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSUTF8StringEncoding];
}

@interface IronApplication : NSApplication {
}
- (void)terminate:(id)sender;
@end

@interface IronAppDelegate : NSObject <NSWindowDelegate> {
}
- (void)windowWillClose:(NSNotification *)notification;
- (void)windowDidResize:(NSNotification *)notification;
- (void)windowWillMiniaturize:(NSNotification *)notification;
- (void)windowDidDeminiaturize:(NSNotification *)notification;
- (void)windowDidResignMain:(NSNotification *)notification;
- (void)windowDidBecomeMain:(NSNotification *)notification;
@end

static IronAppDelegate *delegate;

CAMetalLayer *get_metal_layer(void) {
	return [view metalLayer];
}

id get_metal_device(void) {
	return [view metalDevice];
}

id get_metal_queue(void) {
	return [view metalQueue];
}

bool iron_internal_handle_messages(void) {
	NSEvent *event = [myapp nextEventMatchingMask:NSAnyEventMask
	                                    untilDate:[NSDate distantPast]
	                                       inMode:NSDefaultRunLoopMode
	                                      dequeue:YES]; // distantPast: non-blocking
	if (event != nil) {
		[myapp sendEvent:event];
		[myapp updateWindows];
	}

	// Sleep for a frame to limit the calls when the window is not visible.
	if (!window.visible) {
		[NSThread sleepForTimeInterval:1.0 / 60];
	}
	return true;
}

static void createWindow(iron_window_options_t *options) {
	int width     = options->width / [[NSScreen mainScreen] backingScaleFactor];
	int height    = options->height / [[NSScreen mainScreen] backingScaleFactor];
	int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
	if ((options->features & IRON_WINDOW_FEATURES_RESIZABLE) || (options->features & IRON_WINDOW_FEATURES_MAXIMIZABLE)) {
		styleMask |= NSWindowStyleMaskResizable;
	}
	if (options->features & IRON_WINDOW_FEATURES_MINIMIZABLE) {
		styleMask |= NSWindowStyleMaskMiniaturizable;
	}

	view = [[BasicMTKView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
	[view registerForDraggedTypes:[NSArray arrayWithObjects:NSURLPboardType, nil]];
	window   = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height) styleMask:styleMask backing:NSBackingStoreBuffered defer:TRUE];
	delegate = [IronAppDelegate alloc];
	[window setDelegate:delegate];
	[window setTitle:[NSString stringWithCString:options->title encoding:NSUTF8StringEncoding]];
	[window setAcceptsMouseMovedEvents:YES];
	[[window contentView] addSubview:view];
	[window center];

	windows[0].handle = window;
	windows[0].view   = view;

	[window makeKeyAndOrderFront:nil];

	if (options->mode == IRON_WINDOW_MODE_FULLSCREEN) {
		[window toggleFullScreen:nil];
		windows[0].fullscreen = true;
	}
}

void iron_window_change_window_mode(iron_window_mode_t mode) {
	switch (mode) {
	case IRON_WINDOW_MODE_WINDOW:
		if (windows[0].fullscreen) {
			[window toggleFullScreen:nil];
			windows[0].fullscreen = false;
		}
		break;
	case IRON_WINDOW_MODE_FULLSCREEN:
		if (!windows[0].fullscreen) {
			[window toggleFullScreen:nil];
			windows[0].fullscreen = true;
		}
		break;
	}
}

void iron_window_set_close_callback(bool (*callback)(void *), void *data) {
	windows[0].closeCallback     = callback;
	windows[0].closeCallbackData = data;
}

static void add_menubar(void) {
	NSString *appName = [[NSProcessInfo processInfo] processName];

	NSMenu     *appMenu      = [NSMenu new];
	NSString   *quitTitle    = [@"Quit " stringByAppendingString:appName];
	NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
	[appMenu addItem:quitMenuItem];

	NSMenuItem *appMenuItem = [NSMenuItem new];
	[appMenuItem setSubmenu:appMenu];

	NSMenu *menubar = [NSMenu new];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];
}

void iron_init(iron_window_options_t *win) {
	@autoreleasepool {
		myapp = [IronApplication sharedApplication];
		[myapp finishLaunching];
		[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];
		NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
		add_menubar();

#ifdef WITH_GAMEPAD
		static struct HIDManager *hidManager;
		hidManager = (struct HIDManager *)malloc(sizeof(struct HIDManager));
		HIDManager_init(hidManager);
#endif
	}

	createWindow(win);
	gpu_init(win->depth_bits, true);
}

int iron_window_width(void) {
	NSWindow *window = windows[0].handle;
	float     scale  = [window backingScaleFactor];
	return [[window contentView] frame].size.width * scale;
}

int iron_window_height(void) {
	NSWindow *window = windows[0].handle;
	float     scale  = [window backingScaleFactor];
	return [[window contentView] frame].size.height * scale;
}

void iron_load_url(const char *url) {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

const char *iron_language(void) {
	NSString   *nsstr = [[NSLocale preferredLanguages] objectAtIndex:0];
	const char *lang  = [nsstr UTF8String];
	language[0]       = lang[0];
	language[1]       = lang[1];
	language[2]       = 0;
	return language;
}

void iron_internal_shutdown(void) {}

const char *iron_internal_save_path(void) {
	NSArray  *paths        = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *resolvedPath = [paths objectAtIndex:0];
	NSString *appName      = [NSString stringWithUTF8String:iron_application_name()];
	resolvedPath           = [resolvedPath stringByAppendingPathComponent:appName];

	NSFileManager *fileMgr = [[NSFileManager alloc] init];

	NSError *error;
	[fileMgr createDirectoryAtPath:resolvedPath withIntermediateDirectories:YES attributes:nil error:&error];

	resolvedPath = [resolvedPath stringByAppendingString:@"/"];
	return [resolvedPath cStringUsingEncoding:NSUTF8StringEncoding];
}

#ifndef IRON_NO_MAIN
int main(int argc, char **argv) {
	return kickstart(argc, argv);
}
#endif

@implementation IronApplication

- (void)terminate:(id)sender {
	iron_stop();
}

@end

@implementation IronAppDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
	if (windows[0].closeCallback != NULL) {
		if (windows[0].closeCallback(windows[0].closeCallbackData)) {
			return YES;
		}
		else {
			return NO;
		}
	}
	return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
	iron_stop();
}

void iron_internal_call_resize_callback(int width, int height) {
	if (windows[0].resizeCallback != NULL) {
		windows[0].resizeCallback(width, height, windows[0].resizeCallbackData);
	}
}

- (void)windowDidResize:(NSNotification *)notification {
	NSWindow *window = [notification object];
	NSSize    size   = [[window contentView] frame].size;
	[view resize:size];

	float scale = [window backingScaleFactor];
	int   w     = size.width * scale;
	int   h     = size.height * scale;

	gpu_resize(w, h);
	iron_internal_call_resize_callback(w, h);
}

- (void)windowWillMiniaturize:(NSNotification *)notification {
	iron_internal_background_callback();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
	iron_internal_foreground_callback();
}

- (void)windowDidResignMain:(NSNotification *)notification {
	iron_internal_pause_callback();
}

- (void)windowDidBecomeMain:(NSNotification *)notification {
	iron_internal_resume_callback();
}

@end

int iron_window_x(void) {
	return 0;
}

int iron_window_y(void) {
	return 0;
}

void iron_window_resize(int width, int height) {}
void iron_window_move(int x, int y) {}
void iron_window_change_mode(iron_window_mode_t mode) {}
void iron_window_destroy(void) {}
void iron_window_show(void) {}
void iron_window_hide(void) {}
void iron_window_create(iron_window_options_t *win) {}

void iron_window_set_title(const char *title) {
	NSWindow *window = windows[0].handle;
	[window setTitle:[NSString stringWithUTF8String:title]];
}

void iron_window_set_resize_callback(void (*callback)(int x, int y, void *data), void *data) {
	windows[0].resizeCallback     = callback;
	windows[0].resizeCallbackData = data;
}

iron_window_mode_t iron_window_get_mode(void) {
	return IRON_WINDOW_MODE_WINDOW;
}

int iron_window_display(void) {
	return 0;
}

#ifdef WITH_GAMEPAD

bool iron_gamepad_connected(int num) {
	return true;
}

void iron_gamepad_rumble(int gamepad, float left, float right) {}

static void inputValueCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDValueRef inIOHIDValueRef);
static void valueAvailableCallback(void *inContext, IOReturn inResult, void *inSender);
static void reset(struct HIDGamepad *gamepad);
static void initDeviceElements(struct HIDGamepad *gamepad, CFArrayRef elements);
static void buttonChanged(struct HIDGamepad *gamepad, IOHIDElementRef elementRef, IOHIDValueRef valueRef, int buttonIndex);
static void axisChanged(struct HIDGamepad *gamepad, IOHIDElementRef elementRef, IOHIDValueRef valueRef, int axisIndex);

static int iron_mini(int a, int b) {
	return a > b ? b : a;
}

static void cstringFromCFStringRef(CFStringRef string, char *cstr, size_t clen) {
	cstr[0] = '\0';
	if (string != NULL) {
		char temp[256];
		if (CFStringGetCString(string, temp, 256, kCFStringEncodingUTF8)) {
			temp[iron_mini(255, (int)(clen - 1))] = '\0';
			strncpy(cstr, temp, clen);
		}
	}
}

void HIDGamepad_init(struct HIDGamepad *gamepad) {
	reset(gamepad);
}

void HIDGamepad_destroy(struct HIDGamepad *gamepad) {
	HIDGamepad_unbind(gamepad);
}

void HIDGamepad_bind(struct HIDGamepad *gamepad, IOHIDDeviceRef inDeviceRef, int inPadIndex) {
	gamepad->hidDeviceRef = inDeviceRef;
	gamepad->padIndex     = inPadIndex;

	IOHIDDeviceOpen(gamepad->hidDeviceRef, kIOHIDOptionsTypeSeizeDevice);
	IOHIDDeviceRegisterInputValueCallback(gamepad->hidDeviceRef, inputValueCallback, gamepad);
	IOHIDDeviceScheduleWithRunLoop(gamepad->hidDeviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	gamepad->hidQueueRef = IOHIDQueueCreate(kCFAllocatorDefault, gamepad->hidDeviceRef, 32, kIOHIDOptionsTypeNone);
	if (CFGetTypeID(gamepad->hidQueueRef) == IOHIDQueueGetTypeID()) {
		IOHIDQueueStart(gamepad->hidQueueRef);
		IOHIDQueueRegisterValueAvailableCallback(gamepad->hidQueueRef, valueAvailableCallback, gamepad);
		IOHIDQueueScheduleWithRunLoop(gamepad->hidQueueRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	}

	CFArrayRef elementCFArrayRef = IOHIDDeviceCopyMatchingElements(gamepad->hidDeviceRef, NULL, kIOHIDOptionsTypeNone);
	initDeviceElements(gamepad, elementCFArrayRef);

	{
		CFNumberRef vendorIdRef = (CFNumberRef)IOHIDDeviceGetProperty(gamepad->hidDeviceRef, CFSTR(kIOHIDVendorIDKey));
		CFNumberGetValue(vendorIdRef, kCFNumberIntType, &gamepad->hidDeviceVendorID);

		CFNumberRef productIdRef = (CFNumberRef)IOHIDDeviceGetProperty(gamepad->hidDeviceRef, CFSTR(kIOHIDProductIDKey));
		CFNumberGetValue(productIdRef, kCFNumberIntType, &gamepad->hidDeviceProductID);

		CFStringRef vendorRef = (CFStringRef)IOHIDDeviceGetProperty(gamepad->hidDeviceRef, CFSTR(kIOHIDManufacturerKey));
		cstringFromCFStringRef(vendorRef, gamepad->hidDeviceVendor, sizeof(gamepad->hidDeviceVendor));

		CFStringRef productRef = (CFStringRef)IOHIDDeviceGetProperty(gamepad->hidDeviceRef, CFSTR(kIOHIDProductKey));
		cstringFromCFStringRef(productRef, gamepad->hidDeviceProduct, sizeof(gamepad->hidDeviceProduct));
	}
}

static void initDeviceElements(struct HIDGamepad *gamepad, CFArrayRef elements) {

	for (CFIndex i = 0, count = CFArrayGetCount(elements); i < count; ++i) {
		IOHIDElementRef  elementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		IOHIDElementType elemType   = IOHIDElementGetType(elementRef);

		IOHIDElementCookie cookie = IOHIDElementGetCookie(elementRef);

		uint32_t usagePage = IOHIDElementGetUsagePage(elementRef);
		uint32_t usage     = IOHIDElementGetUsage(elementRef);

		// Match up items
		switch (usagePage) {
		case kHIDPage_GenericDesktop:
			switch (usage) {
			case kHIDUsage_GD_X: // Left stick X
				gamepad->axis[0] = cookie;
				break;
			case kHIDUsage_GD_Y: // Left stick Y
				gamepad->axis[1] = cookie;
				break;
			case kHIDUsage_GD_Z: // Left trigger
				gamepad->axis[4] = cookie;
				break;
			case kHIDUsage_GD_Rx: // Right stick X
				gamepad->axis[2] = cookie;
				break;
			case kHIDUsage_GD_Ry: // Right stick Y
				gamepad->axis[3] = cookie;
				break;
			case kHIDUsage_GD_Rz: // Right trigger
				gamepad->axis[5] = cookie;
				break;
			case kHIDUsage_GD_Hatswitch:
				break;
			default:
				break;
			}
			break;
		case kHIDPage_Button:
			if ((usage >= 1) && (usage <= 15)) {
				// Button 1-11
				gamepad->buttons[usage - 1] = cookie;
			}
			break;
		default:
			break;
		}

		if (elemType == kIOHIDElementTypeInput_Misc || elemType == kIOHIDElementTypeInput_Button || elemType == kIOHIDElementTypeInput_Axis) {
			if (!IOHIDQueueContainsElement(gamepad->hidQueueRef, elementRef))
				IOHIDQueueAddElement(gamepad->hidQueueRef, elementRef);
		}
	}
}

void HIDGamepad_unbind(struct HIDGamepad *gamepad) {
	if (gamepad->hidQueueRef) {
		IOHIDQueueStop(gamepad->hidQueueRef);
		IOHIDQueueUnscheduleFromRunLoop(gamepad->hidQueueRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	}
	if (gamepad->hidDeviceRef) {
		IOHIDDeviceUnscheduleFromRunLoop(gamepad->hidDeviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		IOHIDDeviceClose(gamepad->hidDeviceRef, kIOHIDOptionsTypeSeizeDevice);
	}
	reset(gamepad);
}

static void reset(struct HIDGamepad *gamepad) {
	gamepad->padIndex            = -1;
	gamepad->hidDeviceRef        = NULL;
	gamepad->hidQueueRef         = NULL;
	gamepad->hidDeviceVendor[0]  = '\0';
	gamepad->hidDeviceProduct[0] = '\0';
	gamepad->hidDeviceVendorID   = 0;
	gamepad->hidDeviceProductID  = 0;
	memset(gamepad->axis, 0, sizeof(gamepad->axis));
	memset(gamepad->buttons, 0, sizeof(gamepad->buttons));
}

static void buttonChanged(struct HIDGamepad *gamepad, IOHIDElementRef elementRef, IOHIDValueRef valueRef, int buttonIndex) {
	double rawValue  = IOHIDValueGetScaledValue(valueRef, kIOHIDValueScaleTypePhysical);
	double min       = IOHIDElementGetLogicalMin(elementRef);
	double max       = IOHIDElementGetLogicalMax(elementRef);
	double normalize = (rawValue - min) / (max - min);
	iron_internal_gamepad_trigger_button(gamepad->padIndex, buttonIndex, normalize);
}

static void axisChanged(struct HIDGamepad *gamepad, IOHIDElementRef elementRef, IOHIDValueRef valueRef, int axisIndex) {
	double rawValue  = IOHIDValueGetScaledValue(valueRef, kIOHIDValueScaleTypePhysical);
	double min       = IOHIDElementGetPhysicalMin(elementRef);
	double max       = IOHIDElementGetPhysicalMax(elementRef);
	double normalize = normalize = (((rawValue - min) / (max - min)) * 2) - 1;
	if (axisIndex % 2 == 1) {
		normalize = -normalize;
	}
	iron_internal_gamepad_trigger_axis(gamepad->padIndex, axisIndex, normalize);
}

static void inputValueCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDValueRef inIOHIDValueRef) {}

static void valueAvailableCallback(void *inContext, IOReturn inResult, void *inSender) {
	struct HIDGamepad *pad = (struct HIDGamepad *)inContext;
	do {
		IOHIDValueRef valueRef = IOHIDQueueCopyNextValueWithTimeout((IOHIDQueueRef)inSender, 0.);
		if (!valueRef) {
			break;
		}

		IOHIDElementRef    elementRef = IOHIDValueGetElement(valueRef);
		IOHIDElementCookie cookie     = IOHIDElementGetCookie(elementRef);

		for (int i = 0; i < 15; ++i) {
			if (cookie == pad->buttons[i]) {
				buttonChanged(pad, elementRef, valueRef, i);
				break;
			}
		}

		for (int i = 0; i < 6; ++i) {
			if (cookie == pad->axis[i]) {
				axisChanged(pad, elementRef, valueRef, i);
				break;
			}
		}

		CFRelease(valueRef);
	} while (1);
}

const char *iron_gamepad_vendor(int gamepad) {
	return "unknown";
}

const char *iron_gamepad_product_name(int gamepad) {
	return "unknown";
}

static int                    initHIDManager(struct HIDManager *manager);
static bool                   addMatchingArray(struct HIDManager *manager, CFMutableArrayRef matchingCFArrayRef, CFDictionaryRef matchingCFDictRef);
static CFMutableDictionaryRef createDeviceMatchingDictionary(struct HIDManager *manager, uint32_t inUsagePage, uint32_t inUsage);
static void                   deviceConnected(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
static void                   deviceRemoved(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);

void HIDManager_init(struct HIDManager *manager) {
	manager->managerRef = 0x0;
	initHIDManager(manager);
}

void HIDManager_destroy(struct HIDManager *manager) {
	if (manager->managerRef) {
		IOHIDManagerUnscheduleFromRunLoop(manager->managerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		IOHIDManagerClose(manager->managerRef, kIOHIDOptionsTypeNone);
	}
}

static int initHIDManager(struct HIDManager *manager) {
	manager->managerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (CFGetTypeID(manager->managerRef) == IOHIDManagerGetTypeID()) {

		CFMutableArrayRef matchingCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
		if (matchingCFArrayRef) {
			CFDictionaryRef matchingCFDictRef = createDeviceMatchingDictionary(manager, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);
			addMatchingArray(manager, matchingCFArrayRef, matchingCFDictRef);
			matchingCFDictRef = createDeviceMatchingDictionary(manager, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
			addMatchingArray(manager, matchingCFArrayRef, matchingCFDictRef);
		}
		else {
			iron_error("%s: CFArrayCreateMutable failed.", __PRETTY_FUNCTION__);
			return -1;
		}

		IOHIDManagerSetDeviceMatchingMultiple(manager->managerRef, matchingCFArrayRef);
		CFRelease(matchingCFArrayRef);
		IOHIDManagerOpen(manager->managerRef, kIOHIDOptionsTypeNone);
		IOHIDManagerRegisterDeviceMatchingCallback(manager->managerRef, deviceConnected, manager);
		IOHIDManagerRegisterDeviceRemovalCallback(manager->managerRef, deviceRemoved, manager);
		IOHIDManagerScheduleWithRunLoop(manager->managerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		return 0;
	}
	return -1;
}

bool addMatchingArray(struct HIDManager *manager, CFMutableArrayRef matchingCFArrayRef, CFDictionaryRef matchingCFDictRef) {
	if (matchingCFDictRef) {
		CFArrayAppendValue(matchingCFArrayRef, matchingCFDictRef);
		CFRelease(matchingCFDictRef);
		return true;
	}
	return false;
}

CFMutableDictionaryRef createDeviceMatchingDictionary(struct HIDManager *manager, uint32_t inUsagePage, uint32_t inUsage) {
	CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if (result) {
		if (inUsagePage) {
			CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsagePage);
			if (pageCFNumberRef) {
				CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageCFNumberRef);
				CFRelease(pageCFNumberRef);
				if (inUsage) {
					CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsage);
					if (usageCFNumberRef) {
						CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageCFNumberRef);
						CFRelease(usageCFNumberRef);
					}
				}
			}
		}
	}
	return result;
}

void deviceConnected(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
	struct HIDManager             *manager = (struct HIDManager *)inContext;
	struct HIDManagerDeviceRecord *device  = &manager->devices[0];
	for (int i = 0; i < IRON_MAX_HID_DEVICES; ++i, ++device) {
		if (!device->connected) {
			device->connected = true;
			device->device    = inIOHIDDeviceRef;
			HIDGamepad_bind(&device->pad, inIOHIDDeviceRef, i);
			break;
		}
	}
}

void deviceRemoved(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
	struct HIDManager             *manager = (struct HIDManager *)inContext;
	struct HIDManagerDeviceRecord *device  = &manager->devices[0];
	for (int i = 0; i < IRON_MAX_HID_DEVICES; ++i, ++device) {
		if (device->connected && device->device == inIOHIDDeviceRef) {
			device->connected = false;
			device->device    = NULL;
			HIDGamepad_unbind(&device->pad);
			break;
		}
	}
}

#endif

extern void (*iron_save_and_quit)(bool);

bool _save_and_quit_callback_internal(void) {
	bool save = false;
	NSString *title = [window title];
	bool dirty = [title rangeOfString:@"* - ArmorPaint"].location != NSNotFound;
	if (dirty) {
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:@"Project has been modified, save changes?"];
		[alert addButtonWithTitle:@"Yes"];
		[alert addButtonWithTitle:@"Cancel"];
		[alert addButtonWithTitle:@"No"];
		[alert setAlertStyle:NSAlertStyleWarning];
		NSInteger res = [alert runModal];
		if (res == NSAlertFirstButtonReturn) {
			save = true;
		}
		else if (res == NSAlertThirdButtonReturn) {
			save = false;
		}
		else { // Cancel
			return false;
		}
	}
	iron_save_and_quit(save);
	return false;
}

volatile int iron_exec_async_done = 1;

void iron_exec_async(const char *path, char *argv[]) {
	iron_exec_async_done = 0;
	NSTask *task = [[NSTask alloc] init];
	[task setLaunchPath:[NSString stringWithUTF8String:path]];
	NSMutableArray *args = [NSMutableArray array];
	int i = 1;
	while (argv[i] != NULL) {
		[args addObject:[NSString stringWithUTF8String:argv[i]]];
		i++;
	}
	[task setArguments:args];
	task.terminationHandler = ^(NSTask *t){
		iron_exec_async_done = 1;
	};
	[task launch];
}
