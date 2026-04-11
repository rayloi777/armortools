/*
 * CastleDB Editor - UI Implementation (C++/Objective-C++)
 *
 * This file contains the Dear ImGui UI code which requires C++.
 * It bridges between the C main loop and C++ Dear ImGui.
 */

#ifdef __APPLE__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AppKit/AppKit.h>
#endif

#include "editor_app.h"
#include <stdio.h>

// Dear ImGui
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_metal.h"
#include "imgui/backends/imgui_impl_osx.h"

// Forward declaration for main UI rendering function (defined later in extern "C" block)
extern "C" void editor_render_main_ui(void);

#ifdef __APPLE__

// Forward declaration
@class EditorViewController;

// AppDelegate manages the application lifecycle
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) EditorViewController *viewController;
@end

// EditorViewController handles rendering and input
@interface EditorViewController : NSViewController <MTKViewDelegate>
@property (nonatomic, strong) MTKView *mtkView;
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation EditorViewController

-(instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithNibName:nil bundle:nil];
    if (!self) return nil;

    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];

    if (!self.device) {
        printf("[UI] ERROR: Metal is not supported\n");
        abort();
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup style
    ImGui::StyleColorsDark();

    // Setup Renderer backend
    ImGui_ImplMetal_Init(_device);

    return self;
}

-(MTKView *)mtkView {
    return (MTKView *)self.view;
}

-(void)loadView {
    self.view = [[MTKView alloc] initWithFrame:NSZeroRect];
}

-(void)viewDidLoad {
    [super viewDidLoad];

    self.mtkView.device = self.device;
    self.mtkView.delegate = self;
    self.mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.mtkView.clearColor = MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize OSX platform backend
    ImGui_ImplOSX_Init(self.view);
    [NSApp activateIgnoringOtherApps:YES];
}

-(void)drawInMTKView:(MTKView *)view {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = view.bounds.size.width;
    io.DisplaySize.y = view.bounds.size.height;

    CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: [NSScreen mainScreen].backingScaleFactor;
    io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);

    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];

    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor == nil) {
        [commandBuffer commit];
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplOSX_NewFrame(self.view);
    ImGui::NewFrame();

    // Render editor UI
    editor_render_main_ui();

    // Rendering
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];

    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
}

-(void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}

@end

@implementation AppDelegate

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

-(instancetype)init {
    if (self = [super init]) {
        self.viewController = [[EditorViewController alloc] initWithFrame:NSMakeRect(0, 0, 1200, 800)];

        self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1200, 800)
                                                  styleMask:NSWindowStyleMaskTitled |
                                                            NSWindowStyleMaskClosable |
                                                            NSWindowStyleMaskResizable |
                                                            NSWindowStyleMaskMiniaturizable
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        self.window.contentViewController = self.viewController;
        [self.window center];
        [self.window makeKeyAndOrderFront:self];
    }
    return self;
}

@end

// Global reference to app delegate
static AppDelegate *g_appDelegate = nil;

#endif // __APPLE__

// C linkage exports for the UI functions
extern "C" {

void editor_ui_init(void) {
    printf("[UI] Initializing Dear ImGui\n");

#ifdef __APPLE__
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        g_appDelegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:g_appDelegate];
        [NSApp activateIgnoringOtherApps:YES];
    }
    printf("[UI] Dear ImGui and Metal initialized successfully\n");
#else
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    printf("[UI] Dear ImGui initialized (non-Metal platform)\n");
#endif
}

void editor_ui_shutdown(void) {
    printf("[UI] Shutting down Dear ImGui\n");

#ifdef __APPLE__
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
#endif

    ImGui::DestroyContext();
}

void editor_ui_new_frame(void) {
    // On Apple platforms, rendering is handled in drawInMTKView
    // This is called for consistency but does nothing on Metal
#ifndef __APPLE__
    ImGui::NewFrame();
#endif
}

void editor_run(void) {
#ifdef __APPLE__
    @autoreleasepool {
        [NSApp run];
    }
#endif
}

// Main UI rendering function called from Metal draw loop
void editor_render_main_ui(void) {
    // Render menu bar
    editor_render_menu_bar();

    // Main window
    if (ImGui::Begin("CastleDB Editor", NULL, 0)) {
        // Two-panel layout
        if (ImGui::BeginChild("LeftPanel", ImVec2(200, 0), true)) {
            editor_render_sheet_list();
            ImGui::EndChild();
        }

        ImGui::SameLine();

        if (ImGui::BeginChild("RightPanel", ImVec2(0, 0), true)) {
            editor_render_sheet_table();
            ImGui::EndChild();
        }

        ImGui::End();
    }

    // Column editor dialog
    editor_show_column_editor();

    // About dialog
    if (g_editor.show_about) {
        if (ImGui::Begin("About CastleDB", &g_editor.show_about, 0)) {
            ImGui::Text("CastleDB Editor");
            ImGui::Text("Version: %s", CDB_VERSION);
            ImGui::Separator();
            ImGui::Text("A structured database editor for game data.");
            ImGui::Text("Built with Dear ImGui.");
            ImGui::End();
        }
    }
}

} // extern "C"
