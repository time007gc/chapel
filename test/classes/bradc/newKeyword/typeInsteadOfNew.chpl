type indexType = int(64);

var m: indexType = 3;

class C {
  var myBlock: domain(1, indexType);
  var locDom: [LocalesDomain] domain(1, indexType);
}

const myC = C([1..m]);

