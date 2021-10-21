import * as cp from 'child_process';
import * as path from 'path';
import { safeStorage } from 'electron/main';
import { expect } from 'chai';
import { emittedOnce } from './events-helpers';
import { ifdescribe } from './spec-helpers';
import * as fs from 'fs';

/* 在Linux中，由于模拟dbus而运行CI时，isEncryptionAvailable返回False。这会阻止*Chrome访问系统的密钥环或库机密。当在config.store*设置为Basic-Text的情况下运行测试时，会从Chrome返回一个nullptr，将可用的加密默认为false。**由于所有加密方法都由isEncryptionAvailable门控，所以在CI和Linux上运行时，这些方法永远不会返回正确值*。*/

ifdescribe(process.platform !== 'linux')('safeStorage module', () => {
  after(async () => {
    const pathToEncryptedString = path.resolve(__dirname, 'fixtures', 'api', 'safe-storage', 'encrypted.txt');
    if (fs.existsSync(pathToEncryptedString)) {
      await fs.unlinkSync(pathToEncryptedString);
    }
  });

  describe('SafeStorage.isEncryptionAvailable()', () => {
    it('should return true when encryption key is available (macOS, Windows)', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(true);
    });
  });

  describe('SafeStorage.encryptString()', () => {
    it('valid input should correctly encrypt string', () => {
      const plaintext = 'plaintext';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('UTF-16 characters can be encrypted', () => {
      const plaintext = '€ - utf symbol';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });
  });

  describe('SafeStorage.decryptString()', () => {
    it('valid input should correctly decrypt string', () => {
      const encrypted = safeStorage.encryptString('plaintext');
      expect(safeStorage.decryptString(encrypted)).to.equal('plaintext');
    });

    it('UTF-16 characters can be decrypted', () => {
      const plaintext = '€ - utf symbol';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(safeStorage.decryptString(encrypted)).to.equal(plaintext);
    });

    it('unencrypted input should throw', () => {
      const plaintextBuffer = Buffer.from('I am unencoded!', 'utf-8');
      expect(() => {
        safeStorage.decryptString(plaintextBuffer);
      }).to.throw(Error);
    });

    it('non-buffer input should throw', () => {
      const notABuffer = {} as any;
      expect(() => {
        safeStorage.decryptString(notABuffer);
      }).to.throw(Error);
    });
  });
  describe('safeStorage persists encryption key across app relaunch', () => {
    it('can decrypt after closing and reopening app', async () => {
      const fixturesPath = path.resolve(__dirname, 'fixtures');

      const encryptAppPath = path.join(fixturesPath, 'api', 'safe-storage', 'encrypt-app');
      const encryptAppProcess = cp.spawn(process.execPath, [encryptAppPath]);
      let stdout: string = '';
      encryptAppProcess.stderr.on('data', data => { stdout += data; });
      encryptAppProcess.stderr.on('data', data => { stdout += data; });

      try {
        await emittedOnce(encryptAppProcess, 'exit');

        const appPath = path.join(fixturesPath, 'api', 'safe-storage', 'decrypt-app');
        const relaunchedAppProcess = cp.spawn(process.execPath, [appPath]);

        let output = '';
        relaunchedAppProcess.stdout.on('data', data => { output += data; });
        relaunchedAppProcess.stderr.on('data', data => { output += data; });

        const [code] = await emittedOnce(relaunchedAppProcess, 'exit');

        if (!output.includes('plaintext')) {
          console.log(code, output);
        }
        expect(output).to.include('plaintext');
      } catch (e) {
        console.log(stdout);
        throw e;
      }
    });
  });
});
