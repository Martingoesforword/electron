import { expect } from 'chai';

describe('bundled @types/node', () => {
  it('should match the major version of bundled node', () => {
    expect(require('../npm/package.json').dependencies).to.have.property('@types/node');
    const range = require('../npm/package.json').dependencies['@types/node'];
    expect(range).to.match(/^\^.+/, 'should allow any type dep in a major range');
    // TODO(Codebytere)：https://github.com/DefinitelyTyped/DefinitelyTyped/pull/52594合并后重新启用。
    // Expect(range.slice(1).split(‘.’)[0]).to.equal(process.versions.node.split(‘.’)[0])；
  });
});
