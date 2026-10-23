import os
import sys
import tkinter as tk
from PIL import Image, ImageTk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.dirname(SCRIPT_DIR)
ARTIFACT_DIR = r"C:\Users\Juan Cruz\.gemini\antigravity\brain\2d7c1eb1-f107-4c8f-a5ed-54953630e4b0"

class AuraBooksSimulator(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("AuraBooks Native — Kindle Touch Hardware Simulator (600x800)")
        self.geometry("680x960")
        self.configure(bg="#222326")
        self.resizable(False, False)

        # State
        self.view_mode = "home"
        self.page_num = 142
        self.hud_visible = False

        # Chassis label header
        self.lbl_kindle = tk.Label(self, text="K I N D L E", font=("Helvetica", 10, "bold"), fg="#888a90", bg="#222326")
        self.lbl_kindle.pack(pady=(12, 6))

        # Canvas screen 600x800
        self.canvas = tk.Canvas(self, width=600, height=800, bg="#f7f6f2", highlightthickness=2, highlightbackground="#111")
        self.canvas.pack()
        self.canvas.bind("<Button-1>", self.on_screen_click)

        # Bottom physical Home button
        self.btn_home = tk.Frame(self, width=64, height=32, bg="#33353a", cursor="hand2")
        self.btn_home.pack(pady=(14, 2))
        for _ in range(4):
            line = tk.Frame(self.btn_home, width=44, height=2, bg="#a0a2a8")
            line.pack(pady=2)
        self.btn_home.bind("<Button-1>", lambda e: self.go_home())

        self.lbl_btn = tk.Label(self, text="BOTÓN FÍSICO HOME", font=("Helvetica", 8, "bold"), fg="#666870", bg="#222326")
        self.lbl_btn.pack()

        # Keyboard shortcuts
        self.bind("<Escape>", lambda e: self.go_home())
        self.bind("<Right>", lambda e: self.next_page())
        self.bind("<Left>", lambda e: self.prev_page())

        self.load_images()
        self.render()

    def load_images(self):
        self.imgs = {}
        names = ["home", "library", "reader", "settings"]
        for n in names:
            p = os.path.join(ARTIFACT_DIR, f"native_os_{n}.png")
            if os.path.exists(p):
                self.imgs[n] = Image.open(p).convert("RGB")

    def render(self):
        img = self.imgs.get(self.view_mode, self.imgs.get("home"))
        if img:
            self.tk_img = ImageTk.PhotoImage(img)
            self.canvas.create_image(0, 0, anchor="nw", image=self.tk_img)

    def go_home(self):
        self.view_mode = "home"
        self.render()

    def next_page(self):
        if self.view_mode == "reader":
            self.page_num += 1
            self.render()

    def prev_page(self):
        if self.view_mode == "reader":
            if self.page_num > 1:
                self.page_num -= 1
            self.render()

    def on_screen_click(self, event):
        x, y = event.x, event.y

        # Header: Settings button
        if 488 <= x <= 584 and 14 <= y <= 46 and self.view_mode != "reader":
            self.view_mode = "settings"
            self.render()
            return

        # Settings Modal: Close button
        if self.view_mode == "settings":
            if 160 <= x <= 320 and 550 <= y <= 600:
                self.view_mode = "home"
                self.render()
            return

        # Floating Navbar
        if 732 <= y <= 784 and self.view_mode != "reader":
            if 121 <= x <= 235:
                self.view_mode = "home"
                self.render()
            elif 243 <= x <= 357:
                self.view_mode = "library"
                self.render()
            elif 365 <= x <= 479:
                self.view_mode = "home"
                self.render()
            return

        # Home view: Continuar Lectura
        if self.view_mode == "home":
            if 168 <= x <= 398 and 246 <= y <= 288:
                self.view_mode = "reader"
                self.render()
            elif 32 <= x <= 568 and 616 <= y <= 654:
                self.view_mode = "library"
                self.render()

        # Library view: Tapping any book
        elif self.view_mode == "library":
            for i in range(5):
                by = 102 + i * 90
                if by <= y <= by + 84:
                    self.view_mode = "reader"
                    self.render()
                    break

        # Reader view
        elif self.view_mode == "reader":
            if y < 80:
                self.view_mode = "home"
                self.render()
            elif x > 400:
                self.next_page()
            elif x < 200:
                self.prev_page()

if __name__ == "__main__":
    app = AuraBooksSimulator()
    app.mainloop()
