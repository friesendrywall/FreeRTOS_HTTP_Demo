function buildver()
{
  var file = "inc/buildversion.h";
  var original = CWSys.readStringFromFile(file);
  var text = "#ifndef BUILDVERSION_H\n#define BUILDVERSION_H\n";
  text += '#define HTTP_BUILD_VERSION "' + BUILDVERSION + '"';
  text += "\n#endif";
  if (text != original) {
    CWSys.writeStringToFile(file, text);
  }
}

// Executed when script loaded.
buildver();