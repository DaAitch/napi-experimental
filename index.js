const napiExperimental = require('bindings')('napi_experimental');

napiExperimental.sumAsync(sum => {
  console.log({sum});
});
