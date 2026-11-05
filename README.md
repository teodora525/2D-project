# Autobus — 2D OpenGL projekat

- Autor: Teodora Nikolić
- Indeks: SV66/2022
- Tema: 6 — Autobus
- Jezik: C++
- Grafika: OpenGL 3.3 Core, GLSL 330
- Biblioteke: GLFW i GLAD
- Okruženje: Windows, Visual Studio, MSBuild, x64

> Dokument opisuje poslednju implementaciju sa fullscreen prikazom, putnicima, kontrolorom, kaznama, prilagođenim kursorom i poluprovidnim potpisom. Služi za pokretanje i pripremu odbrane. Ne predstavlja garanciju ocene ili zamenu za originalnu specifikaciju.

## 1. Opis projekta

Aplikacija prikazuje zatvorenu autobusku rutu sa deset stanica, obeleženih brojevima od 0 do 9. Autobus ide redom kroz stanice, a posle stanice 9 vraća se na 0.

Između stanica autobus se kreće konstantnom brzinom po uzorkovanoj putanji. Na svakoj stanici miruje deset sekundi. Dok stoji, vrata su otvorena i moguće je dodavati/uklanjati putnike ili uvesti kontrolora.

Kontrolor putuje do sledeće stanice. Pri dolasku izlazi i generiše se nasumičan broj novih kazni koji se dodaje ukupnom broju.

## 2. Kontrole

| Kontrola | Akcija | Uslov |
|---|---|---|
| Levi klik | Ulazak jednog putnika | Vrata otvorena i manje od 50 putnika |
| Desni klik | Izlazak jednog običnog putnika | Vrata otvorena i postoji običan putnik |
| K | Ulazak kontrolora | Vrata otvorena, slobodno mesto i nema druge kontrole |
| Escape | Izlazak iz aplikacije | Aplikacija prima tastaturni ulaz |

Klikovi važe bilo gde u sadržaju prozora. Držanje dugmeta ne dodaje putnika u svakom frejmu.

## 3. Raspored ekrana

- Sredina: autobuska ruta, stanice i autobus.
- Gore levo: broj putnika uz osobu i ukupan broj kazni uz žuti tiket.
- Gore desno: vrata; tokom stajanja prikazuje se odbrojavanje.
- Dole levo: oznaka kontrolora dok je prisutan.
- Dole desno: poluprovidni potpis Teodora Nikolić, SV66/2022.
- Gore u sredini: izmereni FPS.

## 4. Instalacija i pokretanje

### Preduslovi

- Visual Studio sa opcijom `Desktop development with C++`.
- MSVC i Windows SDK.
- Git i vcpkg.
- Grafički drajver koji podržava OpenGL 3.3 Core.

Projekat je napravljen kao Visual Studio `Empty Project`, ne kao CMake projekat.

### Postavljanje biblioteka

Ako vcpkg još ne postoji:

```powershell
cd $HOME
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe integrate install
.\vcpkg.exe install glfw3:x64-windows glad:x64-windows
```

Ako je već instaliran, preći u njegov direktorijum i izvršiti samo potrebne komande. Ne klonirati ponovo u postojeći folder.

Pri izradi korišćeni su GLFW 3.5.1 i GLAD 0.1.36. Kod koristi `glad/glad.h` i `gladLoadGLLoader`. Komande iznad ne zaključavaju verzije; noviji GLAD API može zahtevati drugačiji kod.

### Pokretanje

1. Otvoriti solution u Visual Studiju.
2. Izabrati `Debug | x64` ili `Release | x64`.
3. Proveriti da postoji samo jedna funkcija `main()`.
4. Pokrenuti sa `Ctrl + F5`.
5. Izaći tasterom `Escape`.

Nisu potrebni FreeType, GLM, stb_image niti putanje do PNG fajlova. Slike su generisane ili ugrađene u kod.

Za rad na drugom računaru proveriti i zavisnosti: sam `.exe` nije nužno dovoljan jer mogu biti potrebni DLL-ovi biblioteka i odgovarajući runtime.

## 5. Konstantne vrednosti

```cpp
constexpr float WIDTH = 1280.0f;
constexpr float HEIGHT = 720.0f;
constexpr double STOP_TIME = 10.0;
constexpr double BUS_SPEED = 110.0;
constexpr bool FULLSCREEN = true;
constexpr double MAX_FPS = 75.0;
```

- `WIDTH` i `HEIGHT`: logička scena, ne fizička rezolucija monitora.
- `STOP_TIME`: stajanje u sekundama.
- `BUS_SPEED`: 110 logičkih jedinica u sekundi, ne km/h.
- `FULLSCREEN`: za predaju ostaje `true`.
- `MAX_FPS`: ciljna gornja granica frejmova.

## 6. Biblioteke i početni redosled

- GLFW: prozor, OpenGL kontekst, događaji, monitor i kursor.
- GLAD: učitavanje pokazivača na OpenGL funkcije.
- OpenGL: iscrtavanje.
- GLSL: jezik šejdera.

Redosled inicijalizacije:

```text
GLFW inicijalizacija
→ prozor i kontekst
→ glfwMakeContextCurrent
→ GLAD inicijalizacija
→ šejderi, geometrija, teksture
→ glavna petlja
→ oslobađanje resursa
```

GLAD se inicijalizuje tek kada je kontekst aktivan.

## 7. Strukture

| Struktura | Uloga |
|---|---|
| `Point` | Koordinate x i y |
| `Mesh` | VAO, VBO i broj temena |
| `Segment` | Uzorkovane tačke deonice, kumulativne dužine i ukupna dužina |
| `State` | Stajanje, putnici, kontrolor, kazne i generator slučajnih brojeva |
| `PixelRun` | Broj ponavljanja i RGBA boja u RLE zapisu slike |

`GLuint` identifikator označava OpenGL resurs. Nije C++ pokazivač na njegovu memoriju.

## 8. VAO, VBO i pajplajn

Pojednostavljen tok:

```text
C++ podaci → VBO/VAO → vertex šejder → primitive
→ rasterizacija → fragment šejder → blending → framebuffer
```

- VBO čuva podatke o temenima.
- VAO pamti način čitanja atributa i njihovu vezu sa baferima.
- Vertex šejder određuje položaj temena.
- Rasterizacija određuje fragmente pokrivene primitivom i interpolira podatke.
- Fragment šejder određuje boju fragmenta.
- Blending kombinuje tu boju sa postojećom bojom u framebufferu.

Ne govoriti da VAO sadrži koordinate: koordinate su u VBO-u.

```cpp
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
```

Parametri označavaju lokaciju atributa 0, dve komponente, tip float, bez normalizacije, razmak `sizeof(Point)` i početni pomeraj 0 u vezanom VBO-u.

Geometrija se šalje jednom sa `GL_STATIC_DRAW`. Kretanje i ponavljanje oblika ostvaruju se uniformama, ne ponovnim slanjem istih temena u svakom frejmu.

## 9. Šejderi i uniforme

### Vertex šejder

```glsl
vec2 p = aPosition * uScale + uOffset;
gl_Position = vec4(
    p.x / 1280.0 * 2.0 - 1.0,
    1.0 - p.y / 720.0 * 2.0,
    0.0,
    1.0
);
vUV = aPosition;
```

Prvo se skaluje lokalni oblik, pa pomera u scenu. Zatim se koordinate pretvaraju u clip koordinate. Pošto je w jednak 1, perspektivna podela ne menja x/y. Nema perspektive: scena je 2D i z je 0.

Za jedinični quad lokalni položaji su 0–1, pa služe i kao UV koordinate. Za neteksturisanu geometriju UV se ne koristi.

### Fragment šejder

```glsl
FragColor = uColor;
if (uTextured)
    FragColor *= texture(uTexture, vUV);
```

Za neteksturisani oblik koristi se uniformna boja. Za teksturu se uzorkovana boja množi uniformnom. Bela uniformna boja čuva izgled slike.

### Uniforme

| Uniforma | Tip | Namena |
|---|---|---|
| `uOffset` | vec2 | Pomeranje |
| `uScale` | vec2 | Dimenzije/skala |
| `uColor` | vec4 | RGBA boja |
| `uTextured` | bool | Izbor teksturisanog prikaza |
| `uTexture` | sampler2D | Teksturna jedinica |

Uniforma ima istu postavljenu vrednost za izvršavanja šejdera u jednom draw pozivu. Atribut položaja se razlikuje po temenu.

Za sampler, vrednost 0 znači teksturna jedinica 0, ne ID teksture. Jedinica se aktivira pomoću `glActiveTexture(GL_TEXTURE0)`, a tekstura vezuje preko `glBindTexture`.

## 10. Koordinate i primitive

Logički početak (0,0) je gore levo; x raste udesno, y nadole.

```text
x_ndc = 2*x/1280 - 1
y_ndc = 1 - 2*y/720
```

- (0,0) postaje (-1,1).
- (640,360) postaje (0,0).
- (1280,720) postaje (1,-1).

`resizeCallback` održava proporcije 16:9. Na drugačijem odnosu stranica mogu ostati trake; fullscreen prozor i dalje pokriva monitor, a krugovi nisu rastegnuti.

- Ruta: `GL_LINE_STRIP`.
- Krugovi: `GL_TRIANGLE_FAN`, centar i obod od 64 segmenta.
- Pravougaonici i teksture: `GL_TRIANGLES`, dva trougla odnosno šest temena.

Isti jedinični quad koristi se za autobus, cifre, panele i potpis. Isti jedinični krug koristi se za stanice i glave ikonica.

## 11. Catmull–Rom ruta

Za deonicu se uzimaju četiri stanice: prethodna p0, trenutna p1, sledeća p2 i naredna p3. Kriva prolazi od p1 do p2, a susedi određuju oblik.

Formula se primenjuje zasebno na x i y:

```text
P(t) = 0.5 * [
    2*p1
    + (-p0+p2)*t
    + (2*p0-5*p1+4*p2-p3)*t²
    + (-p0+3*p1-3*p2+p3)*t³
]
```

`t` je od 0 do 1. Ovo je uniformna Catmull–Rom varijanta, ne Bézier kriva.

Modulo zatvara putanju: `(9+1)%10` je 0. Za prethodnu stanicu koristi se `(i+9)%10`, bez negativnog indeksa.

### Konstantna brzina

Jednaki koraci parametra t ne daju nužno jednake pređene dužine. Zato autobus ne pomeramo samo izrazom `t += brzina*dt`.

Implementacija:

1. Svaku deonicu uzorkuje u 201 tački, odnosno 200 duži.
2. Sabira dužine malih duži i čuva kumulativni niz.
3. Povećava put pomoću `distance += BUS_SPEED * dt`.
4. `upper_bound` pronalazi duž u kojoj je udaljenost.
5. Položaj se linearno interpolira između njenih krajeva.

```text
u = (distance - lengths[i-1]) / (lengths[i] - lengths[i-1])
position = points[i-1] + u*(points[i] - points[i-1])
```

Precizna tvrdnja: brzina je konstantna po uzorkovanoj poliliniji, koja približava matematičku krivu. Ne koristi se tačna analitička dužina luka.

Duže deonice traju duže jer je vreme vožnje `dužina/brzina`.

Za S uzoraka, priprema troši O(10*S) vremena i memorije. Traženje položaja je O(log S), a interpolacija O(1).

## 12. Vreme i stanja

```text
STAJANJE: miruje, vrata otvorena, unos putnika dozvoljen
     ↓ posle 10 sekundi
VOŽNJA: položaj se menja, vrata zatvorena, unos putnika zabranjen
     ↓ dolazak
SLEDEĆA STANICA: kontrolor izlazi, počinje novo stajanje
```

Autobus na početku stoji na stanici 0. `state.stopped` određuje stanje, `stopRemaining` preostalo stajanje, `distance` pređeni put, a `currentStation` trenutnu odnosno početnu stanicu aktivne deonice.

Proteklo vreme računa se kao razlika dva poziva `glfwGetTime()`. Unutrašnja petlja troši celo proteklo vreme, čak i ako ono obuhvati prelaz iz stajanja u vožnju ili dolazak. Ostatak vremena se ne gubi.

Ažuriranje simulacije je pre `glfwPollEvents`, pa klikovi koriste ažurirano stanje.

Nema pauze pri gubitku fokusa. Posle dužeg prekida izvršavanja simulacija može obraditi veći protekli interval.

## 13. Putnici, kontrolor i kazne

`State` je vezan za prozor preko `glfwSetWindowUserPointer`. Callback dobija isti objekat pomoću `glfwGetWindowUserPointer`. GLFW ne kopira objekat niti preuzima vlasništvo. Objekat postoji tokom cele petlje u `main()`.

Callback reaguje na `GLFW_PRESS`, ne na otpuštanje. Za K se ignoriše `GLFW_REPEAT`.

### Pravila brojanja

Broj `passengers` uključuje kontrolora.

```text
obični putnici = passengers - (inspector ? 1 : 0)
0 <= passengers <= 50
inspector == true => passengers >= 1
```

Dok je kontrolor unutra, desni klik može smanjiti broj do 1, ne do 0. Kontrolor mora ostati do sledeće stanice.

K uvodi kontrolora samo tokom stajanja, ako već nije prisutan i postoji mesto. Broj raste za 1.

### Obračun pri dolasku

1. Ako nema kontrole, nema obračuna.
2. Pre izlaska broj običnih putnika je `passengers - 1`.
3. Generiše se broj iz uključivog opsega 0 do tog broja.
4. Rezultat se sabira sa `fines`.
5. Broj putnika se smanjuje za jedan i `inspector` postaje false.

Primer: pet običnih putnika i kontrolor daju broj 6. Generiše se 0–5 kazni, a posle izlaska ostaje pet putnika.

Ako je unutra samo kontrolor, opseg je 0–0 i nema novih kazni.

Koriste se `std::mt19937`, inicijalizovan preko `std::random_device`, i `std::uniform_int_distribution<int>`. Generator je pseudonasumičan i namenjen simulaciji, ne kriptografiji.

### Izbori za nedorečene slučajeve

- Najviše jedan kontrolor istovremeno.
- I kontrolor zauzima jedno od 50 mesta.
- Puno vozilo ne prima kontrolora.
- Desni klik ne izbacuje kontrolora.
- Nova kontrola može ući na stanici na kojoj je stara izašla.

Ovo su odluke implementacije; ne predstavljati ih kao sve doslovno propisane dodatne zahteve.

## 14. Teksture i providnost

| Slika | Način nastanka |
|---|---|
| Cifre 3×5 | Obrasci nula i jedinica |
| Autobus 48×24 | Generisani obojeni pravougaonici |
| Potpis 480×100 | Ugrađeni RLE RGBA podaci |
| Kursor 64×64 | Ugrađeni RLE RGBA podaci |

Vrata i oznaka kontrolora crtaju se geometrijom.

RLE čuva broj ponavljanja i boju uzastopnih jednakih piksela. `decodePixels` pravi običan RGBA niz. To nije PNG dekoder. Dekodiranje se radi jednom pri pokretanju.

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

Za RGB boju:

```text
rezultat = alpha_izvora * boja_izvora
         + (1-alpha_izvora) * boja_pozadine
```

Alfa 0 je potpuno providno, alfa 1 neprovidno. Pozadina potpisa ima alfa 175/255. Sama alfa bez pravilnog blending podešavanja nije dovoljna.

`GL_NEAREST` daje pikselast prikaz cifara i autobusa; `GL_LINEAR` se koristi za potpis. `GL_CLAMP_TO_EDGE` sprečava ponavljanje na ivicama. Mipmape se ne generišu, a filteri ih ne zahtevaju.

Depth test nije uključen. Redosled crtanja je bitan: ruta, stanice, autobus, paneli i potpis. Kasnije nacrtano može prekriti ranije.

## 15. Kursor i fullscreen

`GLFWimage` sadrži RGBA piksele kursora. `glfwCreateCursor` kreira kursor, a `glfwSetCursor` ga povezuje sa prozorom. Hotspot (15,0) je gornji vrh znaka i predstavlja tačku klika. Šipka se pruža dijagonalno nadole udesno.

Kursor nije quad koji se ručno pomera po sceni.

Za fullscreen koristi se primarni monitor i njegov trenutni video režim. Monitor se prosleđuje u `glfwCreateWindow`. `FULLSCREEN=false` omogućava razvoj u prozoru; završna verzija koristi true.

## 16. FPS limiter

VSync je isključen: `glfwSwapInterval(0)`.

Ciljno trajanje frejma je 1/75 sekunde, približno 13.33 ms. Na početku se beleži `frameStart`, a posle iscrtavanja poziva:

```cpp
std::this_thread::sleep_until(frameStart + frameDuration);
```

Ako je obrada kraća, čeka se do roka. Ako je duža, nema dodatnog čekanja. `steady_clock` je monotoni sat za intervale.

Limiter cilja približno najviše 75 FPS. Planer sistema, preciznost spavanja i opterećenje mogu dati niži FPS; nije garantovano idealno tačnih 75.

Delta time i limiter nisu isto: delta time čini simulaciju vremenski doslednom; limiter ograničava učestalost frejmova. Monitor ima sopstvenu frekvenciju osvežavanja i moguć je tearing jer je VSync isključen.

Izmereni FPS je broj frejmova podeljen proteklim vremenom u približno jednosekundnom intervalu. Prikazuje stvarno merenje, ne samo konstantu 75.

## 17. Glavna petlja i oslobađanje

Jedan frejm:

1. Beleženje vremena za limiter.
2. Merenje proteklog vremena simulacije.
3. Ažuriranje stanja, položaja i eventualnog izlaska kontrole.
4. Obrada događaja.
5. Provera izlaska.
6. Ažuriranje FPS-a.
7. Čišćenje framebuffer-a.
8. Iscrtavanje scene i panela.
9. `glfwSwapBuffers`.
10. Čekanje do roka sledećeg frejma.

`glfwSwapBuffers` razmenjuje prednji i zadnji bafer u dvostruko baferovanom prikazu.

Na kraju se uklanjaju callback-ovi, odvaja i uništava kursor, brišu teksture, VAO/VBO i shader program, pa tek onda prozor i GLFW. OpenGL resursi brišu se dok kontekst još postoji.

## 18. Mapa funkcija

| Funkcija | Uloga |
|---|---|
| `errorCallback` | GLFW poruke greške |
| `resizeCallback` | Proporcionalni viewport |
| `mouseCallback` | Ulazak/izlazak običnih putnika |
| `keyCallback` | K i Escape |
| `inspectorLeaves` | Kazne i izlazak kontrole |
| `compileShader` | Kompajliranje i provera šejdera |
| `createProgram` | Povezivanje šejdera |
| `createMesh` / `deleteMesh` | Geometrijski resursi |
| `createTexture` | Kreiranje RGBA teksture |
| `createDigit` / `createBusTexture` | Generisanje slika |
| `decodePixels` | RLE dekodiranje |
| `catmullRom` | Tačka na krivoj |
| `createSegments` | Tačke i dužine deonica |
| `positionAtDistance` | Položaj za zadatu udaljenost |
| `draw` lambda | Uniforme, tekstura, VAO i draw poziv |
| `rect` lambda | Obojeni pravougaonik |
| `number` lambda | Broj preko tekstura cifara |

## 19. Scenario demonstracije

1. Pokrenuti fullscreen i pokazati potpis, kursor, rutu i vrata.
2. Dodati pet putnika, pa jednog ukloniti desnim klikom.
3. Pritisnuti K: broj raste sa 4 na 5 i vidi se kontrolor.
4. Sačekati vožnju; pokazati da klikovi ne menjaju broj.
5. Na sledećoj stanici kontrolor nestaje, broj se vraća na 4 i dodaje se 0–4 kazne.
6. Pokazati da novi K može uvesti novu kontrolu.
7. U kodu objasniti krivu, pređenu dužinu, šejdere i limiter.
8. Izaći pomoću Escape.

Prelazak 9→0 testirati unapred. Ne skraćivati STOP_TIME u verziji koja se predaje.

### Uvodni govor

> Moj projekat je 2D simulacija autobuske rute sa deset stanica. Koristim C++ i OpenGL 3.3 Core, GLFW za prozor i ulaz, a GLAD za učitavanje OpenGL funkcija. Ruta koristi Catmull–Rom krive. Autobus se pomera prema pređenoj dužini po uzorkovanoj putanji i proteklom vremenu, a na svakoj stanici stoji deset sekundi. Dok su vrata otvorena, klikovima menjam broj putnika, a tasterom K uvodim kontrolora. On izlazi na sledećoj stanici i tada se generišu kazne. Aplikacija ima fullscreen, prilagođeni kursor, poluprovidni potpis i limiter podešen na 75 FPS.

## 20. Pitanja za odbranu

### Zašto programabilni pajplajn?

Koristim sopstvene vertex/fragment šejdere, GLSL uniforme i VAO/VBO. Ne koristim `glBegin/glEnd` i fiksni pajplajn.

### VBO naspram VAO?

VBO čuva podatke; VAO opisuje kako se atributi čitaju iz bafera.

### Uniforma naspram atributa?

Atribut se zadaje po temenu, a uniforma ima jednu postavljenu vrednost za izvršavanja šejdera u draw pozivu.

### Kako se autobus pomera bez menjanja VBO-a?

Menja se uniforma `uOffset`, uz isti quad i teksturu.

### Zašto skala pre pomeranja?

Izraz je `aPosition*uScale+uOffset`. Obrnut redosled bi skalirao i pomeraj.

### Zašto ne povećavati samo parametar t?

Jednaka promena t ne znači jednaku dužinu na krivoj. Koristi se kumulativna udaljenost.

### Da li je brzina tačno konstantna po matematičkoj krivoj?

Konstantna je po uzorkovanoj poliliniji; matematička kriva je aproksimirana.

### Kako se meri deset sekundi?

Smanjivanjem `stopRemaining` za proteklo vreme, ne brojanjem frejmova.

### Zašto callback umesto provere pritisnutog miša u svakom frejmu?

Potrebna je jedna akcija po pritisku, a ne mnogo akcija tokom držanja dugmeta.

### Kako callback dobija stanje?

Preko GLFW user pointer-a vezanog za prozor.

### Da li se kontrolor računa u kapacitet?

Da. Povećava broj za jedan i zauzima jedno od 50 mesta. To je izabrano pravilo graničnog slučaja.

### Zašto opseg kazni završava na passengers-1?

Obračun se radi pre izlaska, dok broj još uključuje kontrolora, koji ne dobija kaznu.

### Šta ako je samo kontrolor unutra?

Raspodela je 0–0. Izlazi i ostavlja broj putnika 0.

### Da li se učitavaju PNG slike?

Ne u poslednjoj verziji. Slike su generisane ili ugrađene kao RLE RGBA podaci.

### Zašto ne FreeType?

Brojevi su teksture, a potpis je rasterizovana slika; dodatna biblioteka nije potrebna za postojeći prikaz.

### Da li je fragment isto što i piksel?

Fragment predstavlja potencijalni doprinos pikselu/uzorku i može proći dodatnu obradu pre konačnog upisa.

### Da li alfa sama daje providnost?

Potreban je i uključen odgovarajući blending.

### Zašto 75 FPS ne znači 75 Hz?

FPS je učestalost frejmova aplikacije, a Hz frekvencija monitora.

### Da li limiter garantuje 75 FPS?

Ne. Cilja gornju granicu; opterećenje i Windows tajmeri mogu dati manje.

### Zašto veliki blok podataka u main.cpp?

U njemu su ugrađene slike. Bitno je razumeti format, ne pamtiti njihove numeričke podatke.

### Šta radi NOMINMAX?

Sprečava definisanje Windows min/max makroa kroz zaglavlja koja poštuju tu zastavicu, da ne smetaju `std::min` i sličnim izrazima.

## 21. Checklist testova

Označiti samo ono što je lično provereno:

- [ ] Fullscreen, potpis i kursor rade.
- [ ] Postoji svih 10 numerisanih stanica.
- [ ] Autobus ide redom i prelazi 9→0.
- [ ] Stajanje traje 10 sekundi.
- [ ] Vrata prate stanje autobusa.
- [ ] Levi/desni klik rade samo tokom stajanja.
- [ ] Držanje miša ne ponavlja ulazak.
- [ ] Broj ostaje u opsegu 0–50.
- [ ] K radi samo tokom stajanja i ne uvodi više kontrolora.
- [ ] Puno vozilo ne prima kontrolora.
- [ ] Kontrolor izlazi tek na sledećoj stanici.
- [ ] Nova kontrola može ući na stanici izlaska stare.
- [ ] Sam kontrolor daje 0 novih kazni.
- [ ] Kazne se sabiraju i ne resetuju na svakoj stanici.
- [ ] FPS je približno na granici kada računar to postiže.
- [ ] Escape radi tokom vožnje i stajanja.
- [ ] Provereno pokretanje u konfiguraciji koja se koristi na odbrani.

## 22. Ograničenja i poboljšanja

- Autobus ne rotira prema tangenti krive.
- Na stanici sličica može delimično prekriti broj.
- Ruta koristi tanku liniju podrazumevane širine.
- Simulira se broj putnika, ne pojedinačne osobe i njihove karte.
- Kazne su nasumičan broj, ne stvarna kontrola pojedinačnih karata.
- Nema čuvanja stanja posle izlaska.
- Nema pauze pri gubitku fokusa.
- Veoma veliki zbir kazni može prerasti panel.
- Nema garancije idealno tačnih 75 FPS.
- Jedan veliki fajl je praktičan za kopiranje, ali bi modularni kod bio uredniji.

Moguća poboljšanja: odvojiti renderer i simulaciju, izdvojiti resurse, dodati RAII omotače za OpenGL, bolji font i tekstualne oznake, rotaciju autobusa i testove logike.

## 23. Česte greške i Git

### LNK1168

Prethodni program je možda još pokrenut i izvršni fajl zaključan. Zatvoriti prozor ili odgovarajući proces, pa ponovo pokrenuti build. Ako se problem nastavi, proveriti druge uzroke zaključavanja i dozvole.

### Ne nalazi glad/glad.h ili GLFW/glfw3.h

Proveriti instalaciju paketa, `vcpkg integrate install` i x64 platformu. Biblioteke i projekat moraju biti odgovarajuće arhitekture.

### Git pokušava da doda .vsidx

Dodati pravila u `.gitignore`:

```gitignore
.vs/
*.user
*.suo
[Dd]ebug/
[Rr]elease/
x64/
x86/
*.obj
*.pdb
*.ilk
*.idb
*.pch
*.tlog
```

Ne izostavljati izvorni kod, solution, `.vcxproj` i `.vcxproj.filters`. Već praćene fajlove `.gitignore` ne uklanja automatski iz indeksa.

## 24. Pred predaju

- [ ] Poslednja testirana verzija je sačuvana u Git-u.
- [ ] FULLSCREEN je true, STOP_TIME 10.0, MAX_FPS 75.0.
- [ ] Potpis i indeks su tačni.
- [ ] Tu su projektni fajlovi i README.
- [ ] Lokalni IDE keš i build rezultati nisu u repozitorijumu.
- [ ] Postoji rezervna kopija.
- [ ] Proveren dogovoreni način predaje i termin prema obaveštenjima predmeta.
- [ ] Proveren rad na računaru za odbranu, ako je moguće.
- [ ] Mogu da objasnim i malo izmenim kod pred asistentom.

## 25. Redosled učenja

1. Kontrole i uvodni govor.
2. Stanja, putnici i kontrolor.
3. Delta time, kriva i pređena dužina.
4. VAO/VBO, šejderi i uniforme.
5. Teksture, blending i RLE.
6. Fullscreen, kursor, limiter i oslobađanje resursa.
7. Samostalna demonstracija i pitanja bez gledanja u dokument.

Važno je razlikovati zahteve zadatka, način na koji ih kod realizuje i sopstvene odluke za granične slučajeve.
