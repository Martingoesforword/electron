const args = require('minimist')(process.argv.slice(2));
const { Octokit } = require('@octokit/rest');
const octokit = new Octokit();

async function checkIfDocOnlyChange () {
  if (args.prNumber || args.prBranch || args.prURL) {
    try {
      let pullRequestNumber = args.prNumber;
      if (!pullRequestNumber || isNaN(pullRequestNumber)) {
        if (args.prURL) {
          // CircleCI不提供分支构建的PR编号，但它提供PR URL。
          const pullRequestParts = args.prURL.split('/');
          pullRequestNumber = pullRequestParts[pullRequestParts.length - 1];
        } else if (args.prBranch) {
          // AppVeyor不提供分支机构建设的PR号-从分支机构中找出。
          const prsForBranch = await octokit.pulls.list({
            owner: 'electron',
            repo: 'electron',
            state: 'open',
            head: `electron:${args.prBranch}`
          });
          if (prsForBranch.data.length === 1) {
            pullRequestNumber = prsForBranch.data[0].number;
          } else {
            // 如果分支机构上有0个请购单或多个请购单，只需假设这不仅仅是单据更改
            process.exit(1);
          }
        }
      }
      const filesChanged = await octokit.paginate(octokit.pulls.listFiles.endpoint.merge({
        owner: 'electron',
        repo: 'electron',
        pull_number: pullRequestNumber,
        per_page: 100
      }));

      console.log('Changed Files:', filesChanged.map(fileInfo => fileInfo.filename));

      const nonDocChange = filesChanged.find((fileInfo) => {
        const fileDirs = fileInfo.filename.split('/');
        if (fileDirs[0] !== 'docs') {
          return true;
        }
      });
      if (nonDocChange || filesChanged.length === 0) {
        process.exit(1);
      } else {
        process.exit(0);
      }
    } catch (ex) {
      console.error('Error getting list of files changed: ', ex);
      process.exit(-1);
    }
  } else {
    console.error(`Check if only the docs were changed for a commit.
    Usage: doc-only-change.js --prNumber=PR_NUMBER || --prBranch=PR_BRANCH || --prURL=PR_URL`);
    process.exit(-1);
  }
}

checkIfDocOnlyChange();
