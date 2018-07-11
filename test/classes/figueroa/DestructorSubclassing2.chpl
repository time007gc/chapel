pragma "use default init"
class C1 {
  var x: int;
  proc deinit () {writeln("Inside ~C1");}
}

pragma "use default init"
class C2 {
  var y: int;
  proc deinit () {writeln("Inside ~C2");}
}

pragma "use default init"
class C: C1, C2 {
  proc deinit () {writeln("Inside ~C");}
}

var c = new C();
delete c;
