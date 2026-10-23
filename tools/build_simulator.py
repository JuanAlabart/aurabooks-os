import os
import base64

ARTIFACT_DIR = r"C:\Users\Juan Cruz\.gemini\antigravity\brain\2d7c1eb1-f107-4c8f-a5ed-54953630e4b0"
COVERS_DIR = r"c:\Users\Juan Cruz\Downloads\ING INV\aurabooks_native\assets\covers"

def get_b64(filename):
    p = os.path.join(COVERS_DIR, filename)
    if os.path.exists(p):
        with open(p, "rb") as f:
            return "data:image/jpeg;base64," + base64.b64encode(f.read()).decode("ascii")
    return ""

cov_crimen = get_b64("crimen_y_castigo.jpg")
cov_extranjero = get_b64("el_extranjero.jpg")
cov_jugador = get_b64("el_jugador.jpg")
cov_metamorfosis = get_b64("la_metamorfosis.jpg")
cov_sombra = get_b64("la_sombra_del_viento.jpg")

html_content = f"""<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <title>AuraBooks Native — Kindle Touch Simulator</title>
  <script src="https://www.gstatic.com/antigravity/web/dev/tailwindcss.min.js"></script>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Literata:ital,opsz,wght@0,7..72,400;0,7..72,600;0,7..72,700;1,7..72,400&family=Inter:wght@400;500;600;700&display=swap');
    
    body {{
      font-family: 'Literata', serif;
      user-select: none;
      -webkit-user-select: none;
    }}
    .sans {{
      font-family: 'Inter', sans-serif;
    }}
    /* E-Ink Waveform Flashes */
    .flash-fast {{
      animation: einkFast 0.18s ease-in-out;
    }}
    .flash-full {{
      animation: einkFull 0.45s ease-in-out;
    }}
    @keyframes einkFast {{
      0% {{ opacity: 1; }}
      30% {{ opacity: 0.2; filter: contrast(200%); }}
      100% {{ opacity: 1; }}
    }}
    @keyframes einkFull {{
      0% {{ filter: invert(0%); }}
      25% {{ filter: invert(100%); }}
      50% {{ filter: invert(0%); }}
      75% {{ filter: invert(100%); }}
      100% {{ filter: invert(0%); }}
    }}
    /* Bezel Hardware styling */
    .kindle-bezel {{
      width: 680px;
      height: 980px;
      background: radial-gradient(circle at 50% 30%, #3a3b3e 0%, #222326 100%);
      border-radius: 36px;
      box-shadow: 0 30px 60px rgba(0,0,0,0.45), inset 0 2px 4px rgba(255,255,255,0.15), inset 0 -4px 8px rgba(0,0,0,0.6);
      position: relative;
    }}
    .kindle-screen {{
      width: 600px;
      height: 800px;
      background-color: #f7f6f2; /* Authentic E-Ink Pearl warm paper */
      border-radius: 6px;
      box-shadow: inset 0 3px 8px rgba(0,0,0,0.35);
      position: relative;
      overflow: hidden;
    }}
    /* 4-Stripe Home Button */
    .home-button {{
      width: 64px;
      height: 32px;
      display: flex;
      flex-direction: column;
      justify-content: space-evenly;
      align-items: center;
      cursor: pointer;
      opacity: 0.65;
      transition: opacity 0.15s, transform 0.1s;
    }}
    .home-button:hover {{
      opacity: 0.95;
    }}
    .home-button:active {{
      transform: scale(0.94);
    }}
    .home-stripe {{
      width: 44px;
      height: 2.5px;
      background-color: #a0a2a8;
      border-radius: 1px;
      box-shadow: 0 1px 1px rgba(0,0,0,0.4);
    }}
  </style>
</head>
<body class="bg-slate-900 min-h-screen flex flex-col items-center justify-center p-6 text-slate-800">

  <div class="mb-4 text-center">
    <div class="inline-flex items-center gap-2 bg-slate-800/80 px-4 py-1.5 rounded-full border border-slate-700/60 text-xs text-slate-300 font-mono">
      <span class="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
      AuraBooks Native C OS • Simulador Hardware (Kindle Touch D01200)
    </div>
  </div>

  <!-- Kindle Chassis -->
  <div class="kindle-bezel flex flex-col items-center pt-8 pb-6 px-10">
    
    <!-- Top Kindle Logo Header -->
    <div class="text-[13px] tracking-[0.25em] font-sans font-semibold text-slate-400/50 uppercase mb-4">
      Kindle
    </div>

    <!-- Active 600x800 Framebuffer Screen -->
    <div id="screen" class="kindle-screen text-black flex flex-col">
      <!-- Screen Content Rendered by JS -->
    </div>

    <!-- Bottom Bezel with Physical Home Button -->
    <div class="mt-5 flex flex-col items-center">
      <div class="home-button" onclick="handlePhysicalHome()" title="Botón Físico Home (Kindle Touch)">
        <div class="home-stripe"></div>
        <div class="home-stripe"></div>
        <div class="home-stripe"></div>
        <div class="home-stripe"></div>
      </div>
      <span class="text-[10px] sans text-slate-500 mt-1 uppercase tracking-wider font-semibold">Botón Físico</span>
    </div>

  </div>

  <div class="mt-4 text-xs text-slate-400 sans text-center max-w-lg">
    💡 <strong>Instrucciones:</strong> Haz clic en las pestañas, abre cualquier libro, toca los lados de la pantalla para pasar páginas, abre Ajustes para probar el interruptor de autoarranque, o haz clic en las 4 rayas inferiores (botón Home físico) para volver.
  </div>

  <script>
    const covers = {{
      crimen: "{cov_crimen}",
      extranjero: "{cov_extranjero}",
      jugador: "{cov_jugador}",
      metamorfosis: "{cov_metamorfosis}",
      sombra: "{cov_sombra}"
    }};

    const state = {{
      viewMode: 'SHELL', // 'SHELL', 'READER', 'COVER', 'SETTINGS'
      activeTab: 'home', // 'home', 'library', 'quotes'
      libFilter: 0,
      libPage: 0,
      quotesFilter: 0,
      quotesPage: 0,
      activeBookIdx: 0,
      quoteIdx: 0,
      showReaderHud: false,
      readerFontSize: 16,
      readerPage: 142,
      readerTotalPages: 420,
      pageTurnCount: 0,
      autobootEnabled: true,
      wifiConnected: true,
      wifiSsid: "Wi-Fi Casa",
      wifiIp: "192.168.1.45",
      supabaseStatus: "Listo para sincronizar",
      lastSyncTime: "Hoy 14:50",
      syncedCount: 17,
      isSyncing: false
    }};

    const books = [
      {{ id: 0, title: "Crimen y castigo", author: "Fiódor Dostoyevski", cover: covers.crimen, pct: 33, curPage: 142, totalPages: 420, rating: 5 }},
      {{ id: 1, title: "El extranjero", author: "Albert Camus", cover: covers.extranjero, pct: 79, curPage: 95, totalPages: 120, rating: 5 }},
      {{ id: 2, title: "La metamorfosis", author: "Franz Kafka", cover: covers.metamorfosis, pct: 0, curPage: 0, totalPages: 85, rating: 4 }},
      {{ id: 3, title: "La sombra del viento", author: "Carlos Ruiz Zafón", cover: covers.sombra, pct: 0, curPage: 0, totalPages: 560, rating: 5 }},
      {{ id: 4, title: "El jugador", author: "Fiódor Dostoyevski", cover: covers.jugador, pct: 0, curPage: 0, totalPages: 210, rating: 4 }}
    ];

    const quotes = [
      {{ text: "El misterio de la existencia humana no radica solo en vivir, sino en encontrar algo por lo que vivir.", author: "Fiódor Dostoyevski", book: "Los hermanos Karamázov", isUser: false }},
      {{ text: "En las profundidades del invierno finalmente aprendí que dentro de mí habitaba un verano invencible.", author: "Albert Camus", book: "El verano", isUser: false }},
      {{ text: "A partir de cierto punto no hay retorno. Ese es el punto que hay que alcanzar.", author: "Franz Kafka", book: "Aforismos", isUser: false }},
      {{ text: "Cada libro tiene un alma. El alma de quien lo escribió y de quienes lo leyeron y soñaron con él.", author: "Carlos Ruiz Zafón", book: "La sombra del viento", isUser: true }}
    ];

    const readerTextPages = [
      `A principios de julio, en una época extraordinariamente calurosa, un joven salió de su cuartucho, en el callejón de San Salvador, y lentamente, como si vacilara, se dirigió hacia el puente K...

Había logrado evitar un encuentro con su patrona en la escalera. Su cuarto, situado bajo el tejado de una alta casa de cinco pisos, más parecía una alacena que una habitación.

La patrona, de quien había alquilado aquella covacha con comida y servicio, vivía un piso más abajo, y cada vez que él salía a la calle le era forzoso pasar por delante de su cocina, cuya puerta casi siempre daba de par en par a la escalera.

Y cada vez que pasaba por allí, el joven experimentaba una sensación desagradable y humillante, de la que se avergonzaba y que le hacía fruncir las cejas con rabia.`,
      `Debía una cantidad considerable a la patrona y temía tropezar con ella. No era que estuviera asustado ni acobardado, ni mucho menos; pero desde hacía algún tiempo se hallaba en un estado de irritación y de tensión nerviosa rayano en la hipocondría.

Vivía tan reconcentrado en sí mismo y tan aislado de todo el mundo, que temía cualquier encuentro, no sólo con su patrona. Estaba aplastado por la pobreza; pero aun sus apuradas circunstancias económicas habían dejado últimamente de preocuparle.

De sus asuntos cotidianos no se ocupaba en absoluto ni quería ocuparse. En realidad, ninguna patrona podía inspirarle temor, pensara lo que pensase hacer. Pero detenerse en la escalera, verse obligado a escuchar todas aquellas tonterías sobre asuntos vulgares...`,
      `...y aguantar las quejas, las amenazas, los lloriqueos, y tener que inventar disculpas, mentir... ¡no, más valía deslizarse como un gato por la escalera y desaparecer sin ser visto por nadie!

Sin embargo, aquella vez, al salir a la calle, él mismo se asombró de su miedo a tropezar con su acreedora.

«¡Quiero intentar una cosa semejante y me asusto de semejantes pequeñeces!», pensó con una sonrisa extraña. «¡Hum!... sí; todo está en manos del hombre y todo se le escapa por pura cobardía... Esto es ya un axioma. Es curioso: ¿qué es lo que más temen las personas? Lo que más temen es un paso nuevo, una palabra nueva propia... Pero hablo demasiado. Y como hablo demasiado, no hago nada.»`
    ];

    function triggerWaveform(isFull = false) {{
      const screen = document.getElementById('screen');
      screen.classList.remove('flash-fast', 'flash-full');
      void screen.offsetWidth; // trigger reflow
      screen.classList.add(isFull ? 'flash-full' : 'flash-fast');
    }}

    function renderScreen(refreshFull = false) {{
      triggerWaveform(refreshFull);
      const screen = document.getElementById('screen');

      if (state.viewMode === 'READER') {{
        renderReaderView(screen);
        return;
      }}
      if (state.viewMode === 'COVER') {{
        renderCoverView(screen);
        return;
      }}

      let contentHtml = '';
      if (state.activeTab === 'home') {{
        contentHtml = renderHomeView();
      }} else if (state.activeTab === 'library') {{
        contentHtml = renderLibraryView();
      }} else {{
        contentHtml = renderQuotesView();
      }}

      screen.innerHTML = `
        ${{renderHeader()}}
        <div class="flex-1 px-4 overflow-hidden relative">
          ${{contentHtml}}
        </div>
        ${{renderNavbar()}}
        ${{state.viewMode === 'SETTINGS' ? renderSettingsModal() : ''}}
      `;
    }}

    function renderHeader() {{
      return `
        <div class="flex items-center justify-between px-5 pt-3.5 pb-2 border-b border-black/10">
          <div class="font-bold text-[20px] tracking-wide">
            ${{state.activeTab === 'home' ? 'A U R A B O O K S' : state.activeTab === 'library' ? 'B I B L I O T E C A' : 'C I T A S'}}
          </div>
          <div class="flex items-center gap-4">
            <span class="text-xs sans font-semibold text-black/60">88% ⚡ 14:05</span>
            <button onclick="openSettings()" class="border border-black px-3.5 py-1 rounded-full text-xs font-bold hover:bg-black hover:text-white transition-colors">
              ⚙ Ajustes
            </button>
          </div>
        </div>
      `;
    }}

    function renderNavbar() {{
      return `
        <div class="p-3 flex justify-center">
          <div class="bg-white border-2 border-black rounded-full px-2 py-1.5 flex items-center gap-2 shadow-sm">
            <button onclick="switchTab('home')" class="px-5 py-2 rounded-full text-xs font-bold transition-all ${{state.activeTab === 'home' ? 'border-2 border-black bg-[#e9e8e3]' : 'hover:bg-black/5'}}">
              ${{state.activeTab === 'home' ? '* Inicio' : 'Inicio'}}
            </button>
            <button onclick="switchTab('library')" class="px-5 py-2 rounded-full text-xs font-bold transition-all ${{state.activeTab === 'library' ? 'border-2 border-black bg-[#e9e8e3]' : 'hover:bg-black/5'}}">
              ${{state.activeTab === 'library' ? '* Biblioteca' : 'Biblioteca'}}
            </button>
            <button onclick="switchTab('quotes')" class="px-5 py-2 rounded-full text-xs font-bold transition-all ${{state.activeTab === 'quotes' ? 'border-2 border-black bg-[#e9e8e3]' : 'hover:bg-black/5'}}">
              ${{state.activeTab === 'quotes' ? '* Citas' : 'Citas'}}
            </button>
          </div>
        </div>
      `;
    }}

    function renderHomeView() {{
      const b = books[state.activeBookIdx];
      const q = quotes[state.quoteIdx];
      return `
        <div class="flex flex-col gap-3.5 pt-2">
          
          <!-- Hero Card: Lectura Actual (240px) -->
          <div class="border border-black rounded-2xl p-4 bg-white/70 shadow-sm flex gap-4">
            <div class="w-[120px] h-[175px] border border-black rounded-md overflow-hidden bg-black/5 flex-shrink-0 shadow-sm">
              <img src="${{b.cover}}" class="w-full h-full object-cover grayscale" alt="Portada" />
            </div>
            <div class="flex-1 flex flex-col justify-between">
              <div>
                <span class="text-[11px] sans font-bold text-black/60 uppercase tracking-wider block mb-1">
                  * LECTURA EN CURSO • ${{b.pct}}% leído
                </span>
                <h3 class="font-bold text-[19px] leading-snug">${{b.title}}</h3>
                <p class="text-xs text-black/70 mt-0.5">${{b.author}} &nbsp; <span class="font-mono text-black">★★★★★</span></p>
                <p class="text-xs text-black font-semibold mt-3">Página ${{b.curPage}} de ${{b.totalPages}} (${{b.pct}}%)</p>
                
                <div class="w-full bg-white border border-black rounded-full h-2.5 mt-1 overflow-hidden">
                  <div class="bg-black h-full" style="width: ${{b.pct}}%"></div>
                </div>
              </div>

              <div class="flex gap-2.5 mt-3">
                <button onclick="openReader(${{b.id}})" class="flex-1 border-2 border-black py-2 rounded-xl text-xs font-bold text-center hover:bg-black hover:text-white transition-all">
                  > Continuar Lectura
                </button>
                <button onclick="openCover()" class="px-4 border border-black py-2 rounded-xl text-xs font-semibold hover:bg-black hover:text-white transition-all">
                  Portada
                </button>
              </div>
            </div>
          </div>

          <!-- Quote of the Day Card (150px) -->
          <div class="border border-black rounded-xl p-4 bg-white/70 shadow-sm flex flex-col justify-between h-[135px]">
            <div class="flex justify-between items-center">
              <span class="font-bold text-xs uppercase tracking-wider">* CITA DEL DÍA</span>
              <button onclick="nextQuote()" class="border border-black px-2.5 py-0.5 rounded text-[11px] sans font-semibold hover:bg-black hover:text-white">
                Otra cita ↺
              </button>
            </div>
            <p class="text-xs italic leading-relaxed text-black/90 px-1">
              "${{q.text}}"
            </p>
            <p class="text-[11px] text-black/70 text-right sans font-medium">
              — ${{q.author}}, <span class="italic">${{q.book}}</span>
            </p>
          </div>

          <!-- Activity & Stats Card (175px) -->
          <div class="border border-black rounded-xl p-3.5 bg-white/70 shadow-sm flex flex-col justify-between">
            <span class="font-bold text-xs uppercase tracking-wider block mb-2">* TU BIBLIOTECA Y ACTIVIDAD</span>
            <div class="grid grid-cols-3 gap-2">
              <div class="border border-black rounded-xl p-2.5 text-center bg-white">
                <div class="font-bold text-base">5 Libros</div>
                <div class="text-[10px] sans text-black/60">En tu Kindle</div>
              </div>
              <div class="border border-black rounded-xl p-2.5 text-center bg-white">
                <div class="font-bold text-base">2 Leyendo</div>
                <div class="text-[10px] sans text-black/60">En progreso</div>
              </div>
              <div class="border border-black rounded-xl p-2.5 text-center bg-white">
                <div class="font-bold text-base">4 Citas</div>
                <div class="text-[10px] sans text-black/60">Guardadas</div>
              </div>
            </div>
            <button onclick="switchTab('library')" class="mt-2.5 w-full border border-black py-1.5 rounded-lg text-xs font-bold text-center hover:bg-black hover:text-white transition-all">
              Explorar Biblioteca Completa >
            </button>
          </div>

        </div>
      `;
    }}

    function renderLibraryView() {{
      const filters = ["Todos", "Autores", "Leyendo", "Por Leer", "Leídos"];
      return `
        <div class="flex flex-col h-full pt-1">
          <!-- Filters Bar -->
          <div class="flex gap-1.5 pb-2.5">
            ${{filters.map((f, i) => `
              <button onclick="setLibFilter(${{i}})" class="flex-1 py-1.5 border ${{state.libFilter === i ? 'border-2 border-black font-bold bg-[#e9e8e3]' : 'border-black font-semibold'}} rounded-xl text-xs">
                ${{f}}
              </button>
            `).join('')}}
          </div>

          <!-- 5-6 Book Rows (84px each) -->
          <div class="flex flex-col gap-2 flex-1">
            ${{books.map(b => `
              <div onclick="openReader(${{b.id}})" class="border border-black rounded-xl p-1.5 bg-white/70 flex gap-3.5 items-center cursor-pointer hover:bg-white transition-all">
                <div class="w-[52px] h-[74px] border border-black rounded overflow-hidden flex-shrink-0 bg-black/5">
                  <img src="${{b.cover}}" class="w-full h-full object-cover grayscale" />
                </div>
                <div class="flex-1">
                  <h4 class="font-bold text-[15px] leading-tight">${{b.title}}</h4>
                  <p class="text-xs text-black/70 mt-0.5">${{b.author}}</p>
                  <p class="text-[11px] sans font-semibold text-black/60 mt-1">★★★★★ • ${{b.pct}}% leído</p>
                </div>
              </div>
            `).join('')}}
          </div>

          <!-- Pagination -->
          <div class="flex justify-center items-center gap-4 py-3">
            <button class="border border-black px-4 py-1.5 rounded-xl text-xs font-bold bg-white opacity-40 cursor-not-allowed">
              < Anterior
            </button>
            <span class="text-xs font-bold sans">Pág. 1 de 1</span>
            <button class="border border-black px-4 py-1.5 rounded-xl text-xs font-bold bg-white opacity-40 cursor-not-allowed">
              Siguiente >
            </button>
          </div>
        </div>
      `;
    }}

    function renderQuotesView() {{
      const filters = ["Todas", "Mis Citas", "Célebres"];
      return `
        <div class="flex flex-col h-full pt-1">
          <div class="flex gap-2 pb-3">
            ${{filters.map((f, i) => `
              <button class="flex-1 py-1.5 border ${{i === 0 ? 'border-2 border-black font-bold bg-[#e9e8e3]' : 'border-black font-semibold'}} rounded-xl text-xs">
                ${{f}}
              </button>
            `).join('')}}
          </div>

          <div class="flex flex-col gap-3 flex-1">
            ${{quotes.slice(0, 3).map(q => `
              <div class="border border-black rounded-xl p-3 bg-white/70 flex flex-col justify-between">
                <div class="flex justify-between items-center mb-1">
                  <span class="text-[10px] sans font-bold text-black/60 uppercase">${{q.isUser ? '* CITA GUARDADA' : '* LITERATURA CLÁSICA'}}</span>
                  <button class="border border-black px-2 py-0.5 rounded text-[10px] font-bold">Copiar</button>
                </div>
                <p class="text-xs italic py-1 leading-relaxed">"${{q.text}}"</p>
                <p class="text-[11px] sans text-black/70 text-right">— ${{q.author}}, ${{q.book}}</p>
              </div>
            `).join('')}}
          </div>

          <div class="flex justify-center items-center gap-4 py-3">
            <button class="border border-black px-4 py-1.5 rounded-xl text-xs font-bold bg-white opacity-40">
              < Anterior
            </button>
            <span class="text-xs font-bold sans">Pág. 1 de 2</span>
            <button class="border border-black px-4 py-1.5 rounded-xl text-xs font-bold bg-white">
              Siguiente >
            </button>
          </div>
        </div>
      `;
    }}

    function renderReaderView(screen) {{
      const pageIdx = state.pageTurnCount % readerTextPages.length;
      const text = readerTextPages[pageIdx];

      screen.innerHTML = `
        <div class="relative w-full h-full flex flex-col justify-between p-8 select-none" onclick="handleReaderTap(event)">
          
          <!-- Optional HUD overlay -->
          ${{state.showReaderHud ? `
            <div class="absolute top-3 left-4 right-4 bg-white border-2 border-black rounded-2xl p-2.5 flex items-center justify-between shadow-lg z-20" onclick="event.stopPropagation()">
              <button onclick="closeReader()" class="border border-black px-3 py-1.5 rounded-xl text-xs font-bold hover:bg-black hover:text-white">
                < Volver
              </button>
              <span class="font-bold text-xs truncate max-w-[240px]">Crimen y castigo</span>
              <div class="flex items-center gap-1.5">
                <button onclick="changeFontSize(-2)" class="border border-black px-2.5 py-1 rounded-lg text-xs font-bold">A-</button>
                <button onclick="changeFontSize(2)" class="border border-black px-2.5 py-1 rounded-lg text-xs font-bold">A+</button>
              </div>
            </div>
          ` : ''}}

          <!-- Book Body Content -->
          <div class="pt-3 leading-relaxed whitespace-pre-line text-black/95" style="font-size: ${{state.readerFontSize}}px">
            ${{text}}
          </div>

          <!-- Bottom Reader Footer -->
          <div class="pt-4 border-t border-black/10 flex flex-col gap-1">
            <div class="flex justify-between items-center text-[11px] sans text-black/60">
              <span>Crimen y castigo</span>
              <span>Página ${{state.readerPage + pageIdx}} de ${{state.readerTotalPages}} (33%)</span>
            </div>
            <div class="w-full bg-black/10 h-1 rounded-full overflow-hidden">
              <div class="bg-black h-full" style="width: 33%"></div>
            </div>
          </div>

        </div>
      `;
    }}

    function renderCoverView(screen) {{
      screen.innerHTML = `
        <div class="relative w-full h-full flex items-center justify-center bg-black cursor-pointer" onclick="closeCover()">
          <img src="${{books[state.activeBookIdx].cover}}" class="max-w-full max-h-full object-contain grayscale" />
          <div class="absolute bottom-6 bg-white/90 text-black px-4 py-1.5 rounded-full text-xs font-bold sans shadow">
            Toca en cualquier parte para volver
          </div>
        </div>
      `;
    }}

    function renderSettingsModal() {{
      return `
        <div class="absolute inset-0 bg-black/40 flex items-center justify-center p-4 z-30" onclick="closeSettings()">
          <div class="w-[520px] max-h-[720px] bg-[#f7f6f2] border-2 border-black rounded-2xl p-5 shadow-2xl flex flex-col justify-between" onclick="event.stopPropagation()">
            <div>
              <h2 class="font-bold text-lg text-center pb-2.5 border-b border-black/20">Ajustes y Conectividad AuraBooks</h2>
              
              <!-- SECTION 1: Wi-Fi -->
              <div class="py-3 border-b border-black/15">
                <div class="flex justify-between items-center mb-1">
                  <span class="font-bold text-xs uppercase tracking-wider">* CONECTIVIDAD WI-FI</span>
                  <span class="text-[11px] sans font-semibold ${{state.wifiConnected ? 'text-emerald-700' : 'text-slate-500'}}">
                    ${{state.wifiConnected ? '● Conectado (85%)' : '○ Desconectado'}}
                  </span>
                </div>
                <p class="text-xs sans text-black/75">
                  Red: <strong>${{state.wifiConnected ? state.wifiSsid : 'Ninguna'}}</strong> • IP: ${{state.wifiConnected ? state.wifiIp : '0.0.0.0'}}
                </p>
                <button onclick="toggleWifi()" class="mt-2 w-full border border-black py-1.5 rounded-lg text-xs font-semibold hover:bg-black hover:text-white transition-all">
                  ${{state.wifiConnected ? '↺ Reconectar / Escanear Wi-Fi' : 'Activar Antena Wi-Fi (wlan0)'}}
                </button>
              </div>

              <!-- SECTION 2: Supabase Cloud Sync -->
              <div class="py-3 border-b border-black/15">
                <span class="font-bold text-xs uppercase tracking-wider block mb-1">* SINCRONIZACIÓN AURABOOKS APP (SUPABASE)</span>
                <p class="text-[11px] sans text-black/60 font-mono truncate">
                  yidwtxqcmwagxdxrtxuf.supabase.co
                </p>
                <p class="text-xs sans text-black/80 mt-1">
                  Última sincro: <strong>${{state.lastSyncTime}}</strong> • Libros sincronizados: <strong>${{state.syncedCount}}</strong>
                </p>
                <div class="text-[11px] sans text-emerald-800 font-semibold mt-0.5">
                  ${{state.supabaseStatus}}
                </div>

                <button onclick="syncWithSupabase()" class="mt-2.5 w-full border-2 border-black py-2 rounded-xl text-xs font-bold text-center hover:bg-black hover:text-white transition-all flex items-center justify-center gap-2">
                  ${{state.isSyncing ? `
                    <span class="inline-block animate-spin">↺</span> Conectando con Supabase...
                  ` : `
                    ☁ Sincronizar Ahora con AuraBooks App
                  `}}
                </button>
              </div>

              <!-- SECTION 3: Upstart Autoboot -->
              <div class="py-3 border-b border-black/15">
                <span class="font-bold text-xs uppercase tracking-wider block mb-1">* AUTOARRANQUE DEL SISTEMA (UPSTART)</span>
                <p class="text-xs sans text-black/80">
                  Estado: <strong class="${{state.autobootEnabled ? 'text-emerald-700' : 'text-slate-600'}}">
                    ${{state.autobootEnabled ? '[ ACTIVO — Inicia directo en AuraBooks ]' : '[ INACTIVO — Inicia en Amazon ]'}}
                  </strong>
                </p>

                <button onclick="toggleAutoboot()" class="mt-2 w-full border border-black py-1.5 rounded-lg text-xs font-semibold hover:bg-black hover:text-white transition-all">
                  ${{state.autobootEnabled ? 'Desactivar Autoarranque (Volver a Amazon)' : 'Activar Autoarranque Puro AuraBooks'}}
                </button>
              </div>

              <!-- SECTION 4: Hardware Specs -->
              <div class="pt-2 text-[11px] text-black/70 sans space-y-0.5">
                <div>• Dispositivo: <strong>Amazon Kindle Touch (D01200) K5</strong></div>
                <div>• SoC: Freescale i.MX508 Cortex-A8 800MHz • 256MB RAM</div>
                <div>• Binario C Puro: <strong>149 KB</strong> • Consumo RAM: &lt; 12 MB</div>
              </div>

            </div>

            <div class="pt-3 border-t border-black/20 flex justify-center">
              <button onclick="closeSettings()" class="border-2 border-black px-8 py-1.5 rounded-xl text-xs font-bold hover:bg-black hover:text-white transition-all">
                Cerrar Ajustes
              </button>
            </div>
          </div>
        </div>
      `;
    }}

    async function syncWithSupabase() {{
      if (state.isSyncing) return;
      state.isSyncing = true;
      state.supabaseStatus = "Descargando libros y lecturas desde Supabase...";
      renderScreen(false);

      try {{
        const url = "https://yidwtxqcmwagxdxrtxuf.supabase.co/rest/v1/books?select=id,title,author,current_page,total_pages,status,rating,cover&order=title.asc";
        const res = await fetch(url, {{
          headers: {{
            "apikey": "sb_publishable_rIcJwDeBSc24Q-_0NNMyJA_IK04u_YS",
            "Authorization": "Bearer sb_publishable_rIcJwDeBSc24Q-_0NNMyJA_IK04u_YS"
          }}
        }});

        if (res.ok) {{
          const liveBooks = await res.json();
          if (liveBooks && liveBooks.length > 0) {{
            // Map live Supabase books into local books array
            books.length = 0;
            liveBooks.forEach((b, idx) => {{
              const pct = (b.total_pages > 0) ? Math.round((b.current_page / b.total_pages) * 100) : 0;
              let cov = covers.crimen;
              if (b.title.toLowerCase().includes("extranjero")) cov = covers.extranjero;
              else if (b.title.toLowerCase().includes("metamorfosis")) cov = covers.metamorfosis;
              else if (b.title.toLowerCase().includes("jugador")) cov = covers.jugador;
              else if (b.title.toLowerCase().includes("sombra") || b.title.toLowerCase().includes("zaf")) cov = covers.sombra;
              else if (b.cover && b.cover.startsWith("http")) cov = b.cover;

              books.push({{
                id: idx,
                title: b.title,
                author: b.author || "Autor Desconocido",
                cover: cov,
                pct: pct,
                curPage: b.current_page || 0,
                totalPages: b.total_pages || 250,
                rating: b.rating || 5
              }});
            }});

            state.syncedCount = liveBooks.length;
            const now = new Date();
            state.lastSyncTime = now.toLocaleTimeString([], {{hour: '2-digit', minute:'2-digit'}});
            state.supabaseStatus = `✓ Sincronizado: ${{liveBooks.length}} libros actualizados desde tu app`;
          }}
        }} else {{
          state.supabaseStatus = "Error de red al conectar con Supabase";
        }}
      }} catch (err) {{
        console.error("Supabase sync error:", err);
        state.supabaseStatus = "Error de conexión (verificar Wi-Fi)";
      }}

      state.isSyncing = false;
      renderScreen(true);
    }}

    function toggleWifi() {{
      state.wifiConnected = !state.wifiConnected;
      renderScreen(false);
    }}

    // State actions
    function switchTab(t) {{
      state.activeTab = t;
      renderScreen(false);
    }}
    function openReader(bookId) {{
      state.activeBookIdx = bookId;
      state.viewMode = 'READER';
      state.showReaderHud = false;
      state.pageTurnCount = 0;
      renderScreen(true);
    }}
    function closeReader() {{
      state.viewMode = 'SHELL';
      renderScreen(false);
    }}
    function handleReaderTap(e) {{
      const rect = e.currentTarget.getBoundingClientRect();
      const clickY = e.clientY - rect.top;
      const clickX = e.clientX - rect.left;

      if (clickY < 80) {{
        state.showReaderHud = !state.showReaderHud;
        renderScreen(false);
      }} else if (clickX > rect.width * 0.7) {{
        state.pageTurnCount++;
        renderScreen(state.pageTurnCount % 6 === 0);
      }} else if (clickX < rect.width * 0.3) {{
        if (state.pageTurnCount > 0) state.pageTurnCount--;
        renderScreen(false);
      }}
    }}
    function changeFontSize(delta) {{
      state.readerFontSize = Math.max(12, Math.min(28, state.readerFontSize + delta));
      renderScreen(false);
    }}
    function openCover() {{
      state.viewMode = 'COVER';
      renderScreen(true);
    }}
    function closeCover() {{
      state.viewMode = 'SHELL';
      renderScreen(false);
    }}
    function nextQuote() {{
      state.quoteIdx = (state.quoteIdx + 1) % quotes.length;
      renderScreen(false);
    }}
    function setLibFilter(idx) {{
      state.libFilter = idx;
      renderScreen(false);
    }}
    function openSettings() {{
      state.viewMode = 'SETTINGS';
      renderScreen(false);
    }}
    function closeSettings() {{
      state.viewMode = 'SHELL';
      renderScreen(false);
    }}
    function toggleAutoboot() {{
      state.autobootEnabled = !state.autobootEnabled;
      renderScreen(false);
    }}
    function handlePhysicalHome() {{
      if (state.viewMode !== 'SHELL' || state.activeTab !== 'home') {{
        state.viewMode = 'SHELL';
        state.activeTab = 'home';
        renderScreen(true);
      }}
    }}

    // Initial boot
    renderScreen(true);
  </script>
</body>
</html>
"""

# Save to artifacts directory
artifact_path = os.path.join(ARTIFACT_DIR, "aurabooks_simulator.html")
with open(artifact_path, "w", encoding="utf-8") as f:
    f.write(html_content)

# Save copy in repo
repo_path = r"c:\Users\Juan Cruz\Downloads\ING INV\aurabooks_native\tools\simulator.html"
os.makedirs(os.path.dirname(repo_path), exist_ok=True)
with open(repo_path, "w", encoding="utf-8") as f:
    f.write(html_content)

print("Created:", artifact_path)
print("Created:", repo_path)
