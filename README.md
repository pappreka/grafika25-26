# Solar System Explorer (C / OpenGL)

## Projekt leírás

A **Solar System Explorer** egy C nyelven készült, interaktív 3D alkalmazás, amely a Naprendszer bolygóinak megjelenítésére és bejárására szolgál.  
A program **SDL + OpenGL** technológiát használ a grafika és az input kezelésére.

A felhasználó:
- szabadon mozoghat a térben,
- kiválaszthat különböző bolygókat,
- leszállhat azok felszínére,
- és interaktív környezetben fedezheti fel azokat.

---

## Követelmények és megvalósítás

Az alábbi táblázat bemutatja, hogy a projekt mely funkciókat valósítja meg, és azokat mely bolygón / rendszerben használja.

### Alap követelmények

- **Kamerakezelés:**  
  Az egér és billentyűzet segítségével szabadon bejárható a tér.

- **Objektumok:**  
  3D modellek (OBJ) betöltése külső fájlokból.

- **Animáció, mozgatás:**  
  - Bolygók forgása
  - Kamera mozgás
  - Interaktív objektumok (felszíni mód)

- **Textúrák:**  
  Minden bolygó és objektum textúrázott.

- **Fények:**  
  - Állítható fényerő (billentyűkkel)

- **Használati útmutató (F1):**  
  A program tartalmaz UI alapú információs megjelenítést.

---

### Extra funkciók

- **Bonyolultabb animációk:**  
  - Bolygók forgása → minden bolygó  
  - Rover animáció → Mars

- **Fizika:**  
  - Gravitáció és ugrás → Föld, Mars, Vénusz  
  - Dobható kövek → Föld

- **Részecskerendszer:**  
  - Por / légköri részecskék → Vénusz  
  - Víz hullámok → Föld

- **Köd (fog):**  
  - Mars → vörös köd  
  - Vénusz → sűrű sárga köd

- **Átlátszóság:**  
  - Víz felület (alpha blending) → Föld  
  - Gyűrűk (pl. Szaturnusz)

- **Árnyék / világítás:**  
  - OpenGL lighting → Föld felszín

- **Ütközésvizsgálat:**  
  - Bounding box → objektumok (fák, kövek, járművek)

- **Felhasználói felület:**  
  - Szöveges UI (információk, státusz)

- **Objektum kijelölés egérrel:**  
  - Interaktív objektumok kiválasztása

- **Térkép kezelés (heightmap jelleg):**  
  - Procedurális terep → minden leszállható bolygó

- **Procedurális geometria:**  
  - Terrain generálás (sin/cos alapú) → minden felszín

- **Environment mapping:**  
  - Skybox / csillagos ég → globális nézet

---

## Bolygó-specifikus funkciók

### Nap
- Textúrázott gömb

### Merkúr
- Kráteres procedurális felszín
- Nincs atmoszféra

### Vénusz
- Vulkanikus terep
- Láva repedések
- Sűrű köd
- Részecskék (por / savas légkör)

### Föld
- Folyó és vízanimáció
- Hullámok és ripple effekt
- Fák, kövek (modellek)
- Dobható kövek + fizika

### Mars
- Kráteres terep
- Rover modell animáció
- Vörös köd

### Külső bolygók
- Textúrázott modellek
- (pl. Szaturnusz gyűrű)

---

## Technikai megvalósítás

A projekt moduláris felépítésű:

- `renderer.c` – OpenGL renderelés
- `camera.c` – kamera kezelés
- `input.c` – input feldolgozás
- `math3d.c` – matematikai műveletek
- `texture.c` – textúrák betöltése
- `mesh.c` – OBJ modell betöltés
- `solar.c` – bolygók kezelése
- `planet_scene.c` – bolygófelszín logika és effektek

---

## Irányítás

- **W / A / S / D** – mozgás  
- **Egér** – kamera forgatás  
- **Bal klikk / E** – interakció  
- **Számgombok** – bolygó kiválasztása  
- **ESC** – visszalépés  

---

## Assets (modellek és textúrák)

A projekt külső erőforrásokat használ:

- OBJ modellek
- JPG / PNG textúrák

**Google Drive link (assets mappa):**  
https://drive.google.com/drive/folders/1mKv5d_BIFkxcaWuL4ddcqH6XUf1QtCyK

---

## Fordítás és futtatás

```bash
make
./app
