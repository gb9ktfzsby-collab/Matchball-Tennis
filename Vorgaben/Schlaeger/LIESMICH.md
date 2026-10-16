# Vorgaben/Schlaeger – dein Schlägermodell

Laut README 3.4 ist das eigene Schlägermodell **die einzige übernommene Datei**. Hier gehört die
**Originaldatei in voller Qualität** hinein (mit allen Texturen), z. B.

```
Vorgaben/Schlaeger/racket.blend      (bevorzugt)  oder  .fbx / .glb / .gltf / .obj
Vorgaben/Schlaeger/Texturen/...      (falls die Texturen getrennt vorliegen)
```

Solange hier nichts liegt, nimmt `Unreal/blender/scripts/schlaeger.py` als Rückfall
`Vorgaben/racket.js` – das ist das Modell aus dem Browserspiel als eingebettete GLB-Datei
(72 470 Punkte, Texturen auf **1024 × 1024** verkleinert). Für die Unreal-Fassung bitte die
Originaldatei mit den großen Texturen hier ablegen und committen (Git LFS ist eingerichtet).

Das Skript skaliert das Modell auf 68,58 cm (27 Zoll), legt den Ursprung ans Griffende und die
Achse entlang des Schlägers, erzeugt LODs und exportiert `SM_Schlaeger.fbx`.
