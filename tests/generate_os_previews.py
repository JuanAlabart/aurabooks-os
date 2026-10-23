import os
import sys
from PIL import Image, ImageDraw, ImageFont

ARTIFACT_DIR = r"C:\Users\Juan Cruz\.gemini\antigravity\brain\2d7c1eb1-f107-4c8f-a5ed-54953630e4b0"
FONTS_DIR = r"c:\Users\Juan Cruz\Downloads\ING INV\aurabooks_native\assets\fonts"
COVERS_DIR = r"c:\Users\Juan Cruz\Downloads\ING INV\aurabooks_native\assets\covers"

FONT_TITLE_PATH = os.path.join(FONTS_DIR, "Literata-Bold.ttf")
FONT_REGULAR_PATH = os.path.join(FONTS_DIR, "Literata-Regular.ttf")
FONT_ITALIC_PATH = os.path.join(FONTS_DIR, "Literata-Italic.ttf")

font_header = ImageFont.truetype(FONT_TITLE_PATH, 22)
font_title = ImageFont.truetype(FONT_TITLE_PATH, 18)
font_body = ImageFont.truetype(FONT_REGULAR_PATH, 14)
font_body_bold = ImageFont.truetype(FONT_TITLE_PATH, 14)
font_body_small = ImageFont.truetype(FONT_REGULAR_PATH, 12)
font_italic = ImageFont.truetype(FONT_ITALIC_PATH, 13)

def draw_rounded_rect(draw, bbox, radius, fill="white", outline="black", width=1):
    draw.rounded_rectangle(bbox, radius=radius, fill=fill, outline=outline, width=width)

def draw_header(draw, title="A U R A B O O K S"):
    y = 14
    draw.text((20, y + 6), title, font=font_header, fill="black")
    
    # Status
    status_str = "88%  14:05"
    draw.text((395, y + 9), status_str, font=font_body, fill="#444444")
    
    # Settings button
    draw_rounded_rect(draw, (488, y, 488 + 96, y + 32), radius=10, fill="white", outline="black", width=1)
    draw.text((508, y + 8), "Ajustes", font=font_title, fill="black")

def draw_navbar(draw, active_tab="home"):
    pill_w = 368
    pill_h = 52
    pill_x = (600 - pill_w) // 2
    pill_y = 732
    
    draw_rounded_rect(draw, (pill_x, pill_y, pill_x + pill_w, pill_y + pill_h), radius=22, fill="white", outline="black", width=1)
    
    btn_w = 114
    btn_h = 42
    btn_y = pill_y + 5
    gap = 8
    
    tabs = [("home", "Inicio", pill_x + 5),
            ("library", "Biblioteca", pill_x + 5 + btn_w + gap),
            ("quotes", "Citas", pill_x + 5 + 2 * (btn_w + gap))]
    
    for key, label, bx in tabs:
        is_active = (key == active_tab)
        draw_rounded_rect(draw, (bx, btn_y, bx + btn_w, btn_y + btn_h), radius=16, fill="white", outline="black", width=2 if is_active else 1)
        text_w = font_body.getlength(label)
        draw.text((bx + (btn_w - text_w) / 2, btn_y + 12), ("* " if is_active else "") + label, font=font_body, fill="black")

def render_home():
    img = Image.new("L", (600, 800), color=255)
    draw = ImageDraw.Draw(img)
    
    draw_header(draw, "A U R A B O O K S")
    
    # 1. Hero Card (x=16, y=64, w=568, h=240)
    x, y, w, h = 16, 64, 568, 240
    draw_rounded_rect(draw, (x, y, x + w, y + h), radius=16, fill=255, outline=0, width=1)
    
    # Cover thumbnail (120x175)
    cover_path = os.path.join(COVERS_DIR, "crimen_y_castigo.jpg")
    if os.path.exists(cover_path):
        cover = Image.open(cover_path).convert("L").resize((120, 175), Image.Resampling.LANCZOS)
        img.paste(cover, (x + 16, y + 16))
        draw.rectangle((x + 16, y + 16, x + 16 + 120, y + 16 + 175), outline=0, width=1)
    
    info_x = x + 16 + 120 + 16
    draw.text((info_x, y + 18), "* LECTURA EN CURSO  •  33% leido", font=font_body_small, fill=80)
    draw.text((info_x, y + 42), "Crimen y castigo", font=font_title, fill=0)
    draw.text((info_x, y + 72), "Fiódor Dostoyevski   [*****]", font=font_body, fill=80)
    draw.text((info_x, y + 104), "Pagina 142 de 420  (33%)", font=font_body, fill=0)
    
    # Progress Bar
    draw_rounded_rect(draw, (info_x, y + 124, info_x + 380, y + 134), radius=4, fill=255, outline=0, width=1)
    draw.rectangle((info_x + 2, y + 126, info_x + 2 + int(376 * 0.33), y + 132), fill=0)
    
    # Buttons
    btn1_w = int(380 * 0.58)
    btn2_w = 380 - btn1_w - 10
    draw_rounded_rect(draw, (info_x, y + h - 58, info_x + btn1_w, y + h - 16), radius=12, fill=255, outline=0, width=2)
    draw.text((info_x + 50, y + h - 46), "> Continuar", font=font_body_bold, fill=0)
    
    draw_rounded_rect(draw, (info_x + btn1_w + 10, y + h - 58, info_x + btn1_w + 10 + btn2_w, y + h - 16), radius=12, fill=255, outline=0, width=1)
    draw.text((info_x + btn1_w + 45, y + h - 46), "Portada", font=font_body, fill=0)
    
    # 2. Quote Card (y=328, h=150)
    y = 328
    h = 150
    draw_rounded_rect(draw, (x, y, x + w, y + h), radius=14, fill=255, outline=0, width=1)
    draw.text((x + 16, y + 16), "* CITA DEL DIA", font=font_title, fill=0)
    draw_rounded_rect(draw, (x + w - 105, y + 12, x + w - 15, y + 38), radius=8, fill=255, outline=0, width=1)
    draw.text((x + w - 95, y + 17), "Otra cita", font=font_body_small, fill=80)
    
    draw.text((x + 16, y + 48), '"El misterio de la existencia humana no radica solo en vivir,\nsino en encontrar algo por lo que vivir."', font=font_body, fill=0)
    draw.text((x + 16, y + 112), "- Fiódor Dostoyevski, Los hermanos Karamázov", font=font_body_small, fill=80)
    
    # 3. Stats Card (y=502, h=175)
    y = 502
    h = 175
    draw_rounded_rect(draw, (x, y, x + w, y + h), radius=14, fill=255, outline=0, width=1)
    draw.text((x + 16, y + 14), "* TU BIBLIOTECA Y ACTIVIDAD", font=font_title, fill=0)
    
    pill_w = (w - 32 - 16) // 3
    pill_y = y + 42
    pills = [("5 Libros", "En tu Kindle"), ("2 Leyendo", "En progreso"), ("4 Citas", "Guardadas")]
    for i, (val, sub) in enumerate(pills):
        px = x + 16 + i * (pill_w + 8)
        draw_rounded_rect(draw, (px, pill_y, px + pill_w, pill_y + 62), radius=12, fill=255, outline=0, width=1)
        tw = font_title.getlength(val)
        draw.text((px + (pill_w - tw) / 2, pill_y + 10), val, font=font_title, fill=0)
        sw = font_body_small.getlength(sub)
        draw.text((px + (pill_w - sw) / 2, pill_y + 36), sub, font=font_body_small, fill=100)
    
    # Shortcut button
    btn_y = pill_y + 62 + 10
    draw_rounded_rect(draw, (x + 16, btn_y, x + w - 16, btn_y + 38), radius=10, fill=255, outline=0, width=1)
    btn_str = "Explorar Biblioteca Completa >"
    bw = font_title.getlength(btn_str)
    draw.text((x + (w - bw) / 2, btn_y + 8), btn_str, font=font_title, fill=0)
    
    draw_navbar(draw, "home")
    img.save(os.path.join(ARTIFACT_DIR, "native_os_home.png"))
    print("Saved native_os_home.png")

def render_library():
    img = Image.new("L", (600, 800), color=255)
    draw = ImageDraw.Draw(img)
    
    draw_header(draw, "B I B L I O T E C A")
    
    x = 16
    fw = 108
    filters = ["Todos", "Autores", "Leyendo", "Por Leer", "Leidos"]
    for i, f in enumerate(filters):
        is_active = (i == 0)
        draw_rounded_rect(draw, (x + i * (fw + 6), 56, x + i * (fw + 6) + fw, 90), radius=12, fill=255, outline=0, width=2 if is_active else 1)
        tw = font_body.getlength(f)
        draw.text((x + i * (fw + 6) + (fw - tw) / 2, 65), f, font=font_body, fill=0)
        
    books = [
        ("Crimen y castigo", "Fiódor Dostoyevski", "33% leido", "crimen_y_castigo.jpg", 5),
        ("El extranjero", "Albert Camus", "79% leido", "el_extranjero.jpg", 5),
        ("La metamorfosis", "Franz Kafka", "0% leido", "la_metamorfosis.jpg", 4),
        ("La sombra del viento", "Carlos Ruiz Zafón", "0% leido", "la_sombra_del_viento.jpg", 5),
        ("El jugador", "Fiódor Dostoyevski", "0% leido", "el_jugador.jpg", 4),
    ]
    
    card_y = 102
    card_w = 568
    for i, (title, author, pct, cov_file, rating) in enumerate(books):
        y = card_y + i * 90
        draw_rounded_rect(draw, (x, y, x + card_w, y + 84), radius=12, fill=255, outline=0, width=1)
        
        cov_path = os.path.join(COVERS_DIR, cov_file)
        if os.path.exists(cov_path):
            cov = Image.open(cov_path).convert("L").resize((52, 74), Image.Resampling.LANCZOS)
            img.paste(cov, (x + 5, y + 5))
            draw.rectangle((x + 5, y + 5, x + 57, y + 79), outline=0, width=1)
            
        info_x = x + 52 + 16
        draw.text((info_x, y + 10), title, font=font_title, fill=0)
        draw.text((info_x, y + 34), author, font=font_body, fill=100)
        stars = "[*****]" if rating == 5 else "[****-]"
        draw.text((info_x, y + 54), f"{stars}  •  {pct}", font=font_body_small, fill=60)
        
    # Pagination
    draw_rounded_rect(draw, (145, 652, 255, 688), radius=12, fill=255, outline=0, width=1)
    draw.text((162, 661), "< Anterior", font=font_body, fill=0)
    draw.text((270, 661), "Pag. 1 de 1", font=font_body, fill=0)
    draw_rounded_rect(draw, (345, 652, 455, 688), radius=12, fill=255, outline=0, width=1)
    draw.text((362, 661), "Siguiente >", font=font_body, fill=0)
    
    draw_navbar(draw, "library")
    img.save(os.path.join(ARTIFACT_DIR, "native_os_library.png"))
    print("Saved native_os_library.png")

def render_reader():
    img = Image.new("L", (600, 800), color=255)
    draw = ImageDraw.Draw(img)
    
    # Reading text with Literata
    font_reader = ImageFont.truetype(FONT_REGULAR_PATH, 16)
    
    sample_text = [
        "A principios de julio, en una época extraordinariamente calurosa,",
        "un joven salió de su cuartucho, en el callejón de San Salvador,",
        "y lentamente, como si vacilara, se dirigió hacia el puente K...",
        "",
        "Había logrado evitar un encuentro con su patrona en la escalera.",
        "Su cuarto, situado bajo el tejado de una alta casa de cinco pisos,",
        "más parecía una alacena que una habitación.",
        "",
        "La patrona, de quien había alquilado aquella covacha con comida",
        "y servicio, vivía un piso más abajo, y cada vez que él salía",
        "a la calle le era forzoso pasar por delante de su cocina, cuya",
        "puerta casi siempre daba de par en par a la escalera.",
        "",
        "Y cada vez que pasaba por allí, el joven experimentaba una",
        "sensación desagradable y humillante, de la que se avergonzaba",
        "y que le hacía fruncir las cejas con rabia.",
        "",
        "Debía una cantidad considerable a la patrona y temía tropezar con ella."
    ]
    
    margin_x = 36
    cur_y = 48
    line_h = 28
    
    for line in sample_text:
        draw.text((margin_x, cur_y), line, font=font_reader, fill=0)
        cur_y += line_h
        
    # Footer
    footer_y = 800 - 28
    footer_str = "Crimen y castigo  •  Pagina 142 de 420 (33%)"
    fw = font_body_small.getlength(footer_str)
    draw.text(((600 - fw) / 2, footer_y), footer_str, font=font_body_small, fill=80)
    
    # Bottom progress bar
    draw.rectangle((0, 797, int(600 * 0.33), 800), fill=0)
    
    img.save(os.path.join(ARTIFACT_DIR, "native_os_reader.png"))
    print("Saved native_os_reader.png")

def render_settings():
    img = Image.open(os.path.join(ARTIFACT_DIR, "native_os_home.png")).convert("L")
    draw = ImageDraw.Draw(img)
    
    mw, mh = 480, 420
    mx = (600 - mw) // 2
    my = (800 - mh) // 2
    
    draw_rounded_rect(draw, (mx, my, mx + mw, my + mh), radius=16, fill=255, outline=0, width=2)
    
    t = "Ajustes de AuraBooks OS"
    tw = font_title.getlength(t)
    draw.text((mx + (mw - tw) / 2, my + 24), t, font=font_title, fill=0)
    draw.line((mx + 30, my + 60, mx + mw - 30, my + 60), fill=200, width=1)
    
    draw.text((mx + 30, my + 80), "Autoarranque Directo (Upstart):", font=font_title, fill=0)
    draw.text((mx + 30, my + 110), "ESTADO: [ ACTIVO - Inicia en AuraBooks ]", font=font_body, fill=0)
    
    draw_rounded_rect(draw, (mx + 30, my + 140, mx + mw - 30, my + 184), radius=10, fill=255, outline=0, width=1)
    draw.text((mx + 60, my + 152), "Desactivar Autoarranque (Volver a Amazon)", font=font_body_bold, fill=0)
    
    draw.line((mx + 30, my + 210, mx + mw - 30, my + 210), fill=200, width=1)
    draw.text((mx + 30, my + 230), "Informacion de Hardware:", font=font_title, fill=0)
    draw.text((mx + 30, my + 260), "Dispositivo: Amazon Kindle Touch (D01200)", font=font_body, fill=80)
    draw.text((mx + 30, my + 285), "SoC: Freescale i.MX508 Cortex-A8 800MHz, 256MB", font=font_body, fill=80)
    draw.text((mx + 30, my + 310), "Pantalla: E-Ink Pearl 600x800 16-Gray (EPDC)", font=font_body, fill=80)
    
    draw_rounded_rect(draw, (mx + 160, my + mh - 55, mx + 320, my + mh - 15), radius=12, fill=255, outline=0, width=2)
    cw = font_title.getlength("Cerrar")
    draw.text((mx + 160 + (160 - cw) / 2, my + mh - 46), "Cerrar", font=font_title, fill=0)
    
    img.save(os.path.join(ARTIFACT_DIR, "native_os_settings.png"))
    print("Saved native_os_settings.png")

if __name__ == "__main__":
    render_home()
    render_library()
    render_reader()
    render_settings()
