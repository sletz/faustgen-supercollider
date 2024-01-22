z = play { FaustGen.ar(1)!2 * 0.5 }
f = FaustGenController.new(z,1);

// Change to a triangle
(
f.eval( "import(\"stdfaust.lib\"); process = os.triangle(300);");
)

// Play pink noise
(
f.eval("import(\"stdfaust.lib\"); process = _ : ve.korg35LPF(0.4,0.2);");
)

// Change to something more complex
(
f.eval( "import(\"stdfaust.lib\"); process = os.triangle(os.triangle(400) * 200) : _ * 0.2 : ve.korg35LPF(0.4, 0.5);");
)

(
f.eval( "import(\"stdfaust.lib\"); process = no.noise;");
)

(
f.eval( "import(\"stdfaust.lib\"); process = 0,_~+(1):soundfile(\"sound[url:{'levot-all.wav'}]\",2):!,!,_,_;")
)

f << "import(\"stdfaust.lib\"); process = os.triangle(180);";

