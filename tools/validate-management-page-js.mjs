ObjC.import('Foundation');

const sourcePath = 'src/management-pages.c';
const readError = Ref();
const sourceValue = $.NSString.alloc.initWithContentsOfFileEncodingError(
  $(sourcePath),
  $.NSUTF8StringEncoding,
  readError,
);
if (sourceValue === null) {
  throw new Error(`Unable to read ${sourcePath}.`);
}
const source = ObjC.unwrap(sourceValue);
const allocationMatch = source.match(
  /^#define MANAGEMENT_PAGES_ADMIN_PAGE_SIZE (\d+)U?$/m,
);
if (allocationMatch === null) {
  throw new Error('Unable to locate the ADMIN page allocation.');
}
const allocationSize = Number(allocationMatch[1]);
const functionStart = source.indexOf('esp_err_t management_pages_send_admin');
const templateStart = source.indexOf('const int page_length = snprintf', functionStart);
const lengthCheck = source.indexOf('if (page_length', templateStart);

if (functionStart < 0 || templateStart < 0 || lengthCheck < 0) {
  throw new Error(`Unable to locate the ADMIN page template in ${sourcePath}.`);
}

const templateSource = source.slice(templateStart, lengthCheck);
const cStrings = templateSource.match(/"(?:\\.|[^"\\\r\n])*"/g) ?? [];
const page = cStrings.map((literal) => eval(literal)).join('').replace(/%%/g, '%');
const csrfPlaceholder = 'x'.repeat(64);
const renderedPage = page.replace('%s', csrfPlaceholder);
if (renderedPage.includes('%s')) {
  throw new Error('ADMIN page has an unexpected additional string placeholder.');
}
const renderedLength = $.NSString.stringWithString($(renderedPage))
  .lengthOfBytesUsingEncoding($.NSUTF8StringEncoding);
if (renderedLength >= allocationSize) {
  throw new Error(
    `ADMIN page renders to ${renderedLength} bytes, exceeding its ${allocationSize}-byte allocation.`,
  );
}
const scriptStart = page.indexOf('<script>');
const scriptEnd = page.indexOf('</script>', scriptStart);

if (scriptStart < 0 || scriptEnd < 0) {
  throw new Error('Unable to locate the embedded ADMIN-page script.');
}

new Function(page.slice(scriptStart + '<script>'.length, scriptEnd));
const output = $.NSString.stringWithString(
  `Validated embedded ADMIN-page JavaScript and ${renderedLength}-byte rendered page within ${allocationSize} bytes from ${sourcePath}.\n`,
);
$.NSFileHandle.fileHandleWithStandardOutput.writeData(
  output.dataUsingEncoding($.NSUTF8StringEncoding),
);
