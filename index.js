const napiExperimental = require('bindings')('napi_experimental');

const threeTimes = cb => {
  for (let i = 0; i < 3; i++) {
    cb();
  }
};

threeTimes(() => {
  napiExperimental.sumAsync(sum => {
    console.log({title: 'sumAsync', sum});
  });
});

threeTimes(() => {
  napiExperimental.sumAsync2(sum => {
    console.log({title: 'sumAsync2', sum});
  });
});