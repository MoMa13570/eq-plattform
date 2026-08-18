import * as THREE from "https://esm.sh/three@0.166.1";
import { OrbitControls } from "https://esm.sh/three@0.166.1/examples/jsm/controls/OrbitControls.js";

const root = document.getElementById("eq-projection-explainer");

if (root) {
  const container = document.getElementById("eq-projection-3d");
  const phiInput = document.getElementById("eq-projection-phi");
  const betaInput = document.getElementById("eq-projection-beta");
  const phiOutput = document.getElementById("eq-projection-phi-output");
  const betaOutput = document.getElementById("eq-projection-beta-output");
  const phiValue = document.getElementById("eq-projection-phi-value");
  const betaValue = document.getElementById("eq-projection-beta-value");
  const playButton = document.getElementById("eq-projection-play");
  const progress = document.getElementById("eq-projection-progress");
  const viewButtons = [...root.querySelectorAll("[data-view]")];
  const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const styles = getComputedStyle(document.documentElement);
  const cssColor = (name, fallback) => styles.getPropertyValue(name).trim() || fallback;
  const colors = {
    accent: cssColor("--eq-accent", "#087e84"),
    warm: cssColor("--eq-warm", "#e99b38"),
    red: cssColor("--eq-red", "#c8322b"),
    ink: cssColor("--eq-ink", "#13272b"),
    muted: cssColor("--eq-muted", "#5d7074"),
    line: cssColor("--eq-line", "#d4e0de")
  };

  let phi = Number(phiInput.value);
  let beta = Number(betaInput.value);
  let parameterAnimation = null;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(38, 1, 0.1, 100);
  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.setClearColor(0x000000, 0);
  container.replaceChildren(renderer.domElement);

  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.075;
  controls.enablePan = false;
  controls.minDistance = 3.2;
  controls.maxDistance = 10;
  controls.target.set(0, 0, -0.1);

  scene.add(new THREE.HemisphereLight(0xffffff, 0x90a5a4, 2.25));
  const keyLight = new THREE.DirectionalLight(0xffffff, 2.8);
  keyLight.position.set(3, 4, 5);
  scene.add(keyLight);

  const grid = new THREE.GridHelper(5, 10, colors.line, colors.line);
  grid.position.y = -1.45;
  grid.material.transparent = true;
  grid.material.opacity = 0.28;
  scene.add(grid);

  const projectionPlane = new THREE.Mesh(
    new THREE.PlaneGeometry(3.55, 2.7),
    new THREE.MeshBasicMaterial({
      color: colors.accent,
      transparent: true,
      opacity: 0.045,
      side: THREE.DoubleSide,
      depthWrite: false
    })
  );
  projectionPlane.position.z = -0.78;
  scene.add(projectionPlane);

  const planeEdges = new THREE.LineSegments(
    new THREE.EdgesGeometry(projectionPlane.geometry),
    new THREE.LineBasicMaterial({ color: colors.line, transparent: true, opacity: 0.85 })
  );
  planeEdges.position.copy(projectionPlane.position);
  scene.add(planeEdges);

  const roller = new THREE.Mesh(
    new THREE.CylinderGeometry(0.032, 0.032, 3.15, 18),
    new THREE.MeshStandardMaterial({ color: colors.accent, roughness: 0.5, metalness: 0.08 })
  );
  roller.rotation.z = Math.PI / 2;
  roller.position.z = 0.42;
  scene.add(roller);

  const verticalAxis = new THREE.Mesh(
    new THREE.CylinderGeometry(0.012, 0.012, 3.1, 10),
    new THREE.MeshBasicMaterial({ color: colors.muted, transparent: true, opacity: 0.58 })
  );
  verticalAxis.position.z = 0.44;
  scene.add(verticalAxis);

  const dynamicGroup = new THREE.Group();
  scene.add(dynamicGroup);

  const radians = degrees => degrees * Math.PI / 180;
  const formatFactor = value => value.toFixed(3).replace(".", ",");

  function disposeObject(object) {
    if (object.geometry) object.geometry.dispose();
    if (object.material) {
      const materials = Array.isArray(object.material) ? object.material : [object.material];
      materials.forEach(material => material.dispose());
    }
  }

  function clearDynamicGroup() {
    dynamicGroup.traverse(disposeObject);
    dynamicGroup.clear();
  }

  function ellipsePoints(rx, ry, z = 0) {
    return Array.from({ length: 128 }, (_, index) => {
      const angle = index / 128 * Math.PI * 2;
      return new THREE.Vector3(rx * Math.cos(angle), ry * Math.sin(angle), z);
    });
  }

  function makeLineLoop(points, color, opacity = 1, dashed = false) {
    const geometry = new THREE.BufferGeometry().setFromPoints(points);
    const material = dashed
      ? new THREE.LineDashedMaterial({ color, dashSize: 0.075, gapSize: 0.05, transparent: true, opacity })
      : new THREE.LineBasicMaterial({ color, transparent: opacity < 1, opacity });
    const line = new THREE.LineLoop(geometry, material);
    if (dashed) line.computeLineDistances();
    return line;
  }

  function makeEllipseTube(rx, ry) {
    const curve = new THREE.CatmullRomCurve3(ellipsePoints(rx, ry), true, "centripetal");
    return new THREE.Mesh(
      new THREE.TubeGeometry(curve, 128, 0.026, 8, true),
      new THREE.MeshStandardMaterial({ color: colors.warm, roughness: 0.38, metalness: 0.08 })
    );
  }

  function makeSegmentShape(rx, ry, xScale = 1) {
    const left = 0.24;
    const right = 0.62;
    const top = -ry * 0.58;
    const start = Math.acos(right);
    const end = Math.acos(left);
    const shape = new THREE.Shape();
    shape.moveTo(rx * left * xScale, top);
    shape.lineTo(rx * right * xScale, top);
    for (let index = 0; index <= 36; index += 1) {
      const angle = start + (end - start) * index / 36;
      shape.lineTo(rx * Math.cos(angle) * xScale, -ry * Math.sin(angle));
    }
    shape.closePath();
    return shape;
  }

  function makeSegmentMesh(shape, color, opacity = 1) {
    return new THREE.Mesh(
      new THREE.ShapeGeometry(shape, 36),
      new THREE.MeshStandardMaterial({
        color,
        transparent: opacity < 1,
        opacity,
        roughness: 0.48,
        metalness: 0.02,
        side: THREE.DoubleSide,
        depthWrite: opacity === 1
      })
    );
  }

  function updateModel() {
    clearDynamicGroup();

    const cosPhi = Math.cos(radians(phi));
    const cosBeta = Math.max(0.01, Math.cos(radians(beta)));
    const verticalRadius = cosPhi;
    const horizontalRadius = 1 / cosBeta;
    const betaRadians = radians(beta);

    const physicalGroup = new THREE.Group();
    physicalGroup.rotation.y = betaRadians;
    physicalGroup.position.z = 0.22;

    const referenceCircle = makeLineLoop(ellipsePoints(1, 1, -0.015), colors.muted, 0.42, true);
    physicalGroup.add(referenceCircle);
    physicalGroup.add(makeEllipseTube(horizontalRadius, verticalRadius));

    const physicalSegment = makeSegmentMesh(
      makeSegmentShape(horizontalRadius, verticalRadius),
      colors.red,
      0.92
    );
    physicalSegment.position.z = 0.012;
    physicalGroup.add(physicalSegment);
    dynamicGroup.add(physicalGroup);

    const projectedEllipse = makeLineLoop(
      ellipsePoints(horizontalRadius * cosBeta, verticalRadius, -0.765),
      colors.warm,
      0.58
    );
    dynamicGroup.add(projectedEllipse);

    const projectedSegment = makeSegmentMesh(
      makeSegmentShape(horizontalRadius, verticalRadius, cosBeta),
      colors.red,
      0.24
    );
    projectedSegment.position.z = -0.752;
    dynamicGroup.add(projectedSegment);

    const projectedCircle = makeLineLoop(ellipsePoints(1, 1, -0.77), colors.line, 0.8, true);
    dynamicGroup.add(projectedCircle);

    phiValue.textContent = `A = r · cos(φ) = ${formatFactor(cosPhi)} r`;
    betaValue.textContent = `B = r / cos(β) = ${formatFactor(horizontalRadius)} r`;
    phiOutput.textContent = `${Math.round(phi)}°`;
    betaOutput.textContent = `${Math.round(beta)}°`;
  }

  function setView(view) {
    viewButtons.forEach(button => button.setAttribute("aria-pressed", String(button.dataset.view === view)));
    controls.target.set(0, 0, -0.1);
    if (view === "front") {
      camera.up.set(0, 1, 0);
      camera.position.set(0, 0, 5.8);
    } else if (view === "top") {
      camera.up.set(0, 0, -1);
      camera.position.set(0, 5.8, 0.001);
    } else {
      camera.up.set(0, 1, 0);
      camera.position.set(3.7, 2.65, 5.35);
    }
    controls.update();
  }

  function stopParameterAnimation() {
    if (parameterAnimation) cancelAnimationFrame(parameterAnimation);
    parameterAnimation = null;
    playButton.disabled = false;
  }

  phiInput.addEventListener("input", event => {
    stopParameterAnimation();
    phi = Number(event.target.value);
    updateModel();
  });

  betaInput.addEventListener("input", event => {
    stopParameterAnimation();
    beta = Number(event.target.value);
    updateModel();
  });

  playButton.addEventListener("click", () => {
    stopParameterAnimation();
    const targetPhi = Number(phiInput.value);
    const targetBeta = Number(betaInput.value);
    const duration = prefersReducedMotion ? 1 : 4400;
    const start = performance.now();
    playButton.disabled = true;
    setView("perspective");

    const tick = now => {
      const total = Math.min(1, (now - start) / duration);
      if (total < 0.46) {
        phi = targetPhi * (total / 0.46);
        beta = 0;
      } else if (total < 0.56) {
        phi = targetPhi;
        beta = 0;
      } else {
        phi = targetPhi;
        beta = targetBeta * ((total - 0.56) / 0.44);
      }
      progress.style.width = `${total * 100}%`;
      updateModel();

      if (total < 1) {
        parameterAnimation = requestAnimationFrame(tick);
      } else {
        phi = targetPhi;
        beta = targetBeta;
        parameterAnimation = null;
        playButton.disabled = false;
        updateModel();
      }
    };

    parameterAnimation = requestAnimationFrame(tick);
  });

  viewButtons.forEach(button => button.addEventListener("click", () => setView(button.dataset.view)));
  controls.addEventListener("start", () => {
    viewButtons.forEach(button => button.setAttribute("aria-pressed", "false"));
  });

  const resizeObserver = new ResizeObserver(entries => {
    const { width, height } = entries[0].contentRect;
    if (!width || !height) return;
    renderer.setSize(width, height, false);
    camera.aspect = width / height;
    camera.updateProjectionMatrix();
  });
  resizeObserver.observe(container);

  setView("perspective");
  updateModel();

  renderer.setAnimationLoop(() => {
    controls.update();
    renderer.render(scene, camera);
  });
}
